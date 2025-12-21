#include "tls_conn.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace ion {

TlsConnection::TlsConnection(TlsConnection&& other) noexcept
    : client_fd_(std::move(other.client_fd_)), ssl_(std::exchange(other.ssl_, nullptr)) {}

TlsConnection& TlsConnection::operator=(TlsConnection&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    if (ssl_) {
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    client_fd_ = std::move(other.client_fd_);
    ssl_ = std::exchange(other.ssl_, nullptr);
    return *this;
}

TlsConnection::TlsConnection(SocketFd&& client_fd, const std::filesystem::path& cert_path,
                             const std::filesystem::path& key_path)
    : client_fd_(std::move(client_fd)) {
    if (!exists(cert_path)) {
        throw std::runtime_error("certificate file not found");
    }

    if (!exists(key_path)) {
        throw std::runtime_error("private key file not found");
    }

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        throw std::runtime_error("failed to create SSL context");
    }

    if (SSL_CTX_use_certificate_file(ctx, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("failed to load certificate file");
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("failed to load private key file");
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("private key does not match certificate");
    }

    SSL_CTX_set_alpn_select_cb(ctx, alpn_callback, nullptr);

    SSL_CTX_set_keylog_callback(ctx, [](const SSL*, const char* line) {
        if (const auto keylog_file = std::getenv("SSLKEYLOGFILE")) {
            std::ofstream file(keylog_file, std::ios::app);
            if (file.is_open()) {
                file << line << '\n';
            }
        }
    });

    ssl_ = SSL_new(ctx);
    if (!ssl_) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("failed to create SSL object");
    }

    if (SSL_set_fd(ssl_, client_fd_) != 1) {
        throw std::runtime_error("failed to set SSL file descriptor");
    }
}

bool TlsConnection::handshake() const {
    constexpr int timeout_ms = 100;
    constexpr int max_retries = 100;

    for (int attempt = 0; attempt < max_retries; attempt++) {
        const int result = SSL_accept(ssl_);
        if (result == 1) {
            return true;
        }
        switch (const int ssl_error = SSL_get_error(ssl_, result)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE: {
                pollfd pfd = {client_fd_, 0, 0};
                pfd.events = (ssl_error == SSL_ERROR_WANT_READ) ? POLLIN : POLLOUT;
                const int poll_result = poll(&pfd, 1, timeout_ms);
                if (poll_result > 0 && (pfd.revents & pfd.events)) {
                    continue;  // Socket ready, try again
                }
                if (poll_result == 0) {
                    continue;  // Timeout, try again
                }
                if (poll_result < 0) {
                    spdlog::error("poll failed during handshake: {}", strerror(errno));
                    return false;
                }
                break;
            }
            default: {
                char err_buf[256];
                ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
                spdlog::error("SSL handshake failed: {}", err_buf);
                return false;
            }
        }
    }

    spdlog::warn("SSL handshake timed out after {} attempts", max_retries);
    return false;
}

TlsConnection::~TlsConnection() {
    if (ssl_) {
        spdlog::debug("freeing OpenSSL");
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
}

void TlsConnection::graceful_shutdown() const {
    BIO_flush(SSL_get_wbio(ssl_));
    // Bidirectional shutdown
    spdlog::debug("shutting down SSL (client notify)");
    if (const int ret = SSL_shutdown(ssl_); ret == 0) {
        spdlog::debug("shutting down SSL (force)");
        SSL_shutdown(ssl_);
    }
}

// ReSharper disable once CppDFAConstantFunctionResult
int TlsConnection::alpn_callback(SSL*, const unsigned char** out, unsigned char* outlen,
                                 const unsigned char* in, unsigned int inlen, void*) {
    static constexpr std::array<unsigned char, 3> supported_protos{'\x02', 'h', '2'};

    const int result =
        SSL_select_next_proto(const_cast<unsigned char**>(out), outlen, supported_protos.data(),
                              supported_protos.size(), in, inlen);
    if (result == OPENSSL_NPN_NEGOTIATED) {
        spdlog::debug("ALPN negotiated: {}",
                      std::string(reinterpret_cast<const char*>(*out), *outlen));
        return SSL_TLSEXT_ERR_OK;
    }

    *outlen = supported_protos[0];
    *out = supported_protos.data() + 1;
    spdlog::debug("ALPN negotiation failed, using default: {}",
                  std::string(reinterpret_cast<const char*>(*out), *outlen));
    return SSL_TLSEXT_ERR_OK;
}

void TlsConnection::print_debug_to_stderr() {
    ERR_print_errors_fp(stderr);
}

std::expected<ssize_t, TransportError> TlsConnection::read(std::span<uint8_t> buffer) const {
    spdlog::trace("reading from SSL");
    const auto bytes_read = SSL_read(ssl_, buffer.data(), static_cast<int>(buffer.size()));
    if (bytes_read <= 0) {
        switch (const int ssl_error = SSL_get_error(ssl_, bytes_read)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                spdlog::trace("SSL want read/write");
                return std::unexpected(TransportError::WantReadOrWrite);
            case SSL_ERROR_ZERO_RETURN:
                spdlog::debug("TLS connection closed");
                return std::unexpected(TransportError::ConnectionClosed);
            case SSL_ERROR_SSL:
                spdlog::error("TLS protocol error (SSL_ERROR_SSL)");
                return std::unexpected(TransportError::ProtocolError);
            default:
                spdlog::error("TLS read error (SSL_ERROR_*): {}", ssl_error);
                return std::unexpected(TransportError::OtherError);
        }
    }
    spdlog::trace("successfully read {} bytes from SSL", bytes_read);
    return bytes_read;
}

std::expected<ssize_t, TransportError> TlsConnection::write(std::span<const uint8_t> buffer) const {
    const auto bytes_written = SSL_write(ssl_, buffer.data(), static_cast<int>(buffer.size()));
    if (bytes_written < 0) {
        const int ssl_error = SSL_get_error(ssl_, bytes_written);
        spdlog::error("SSL write error: {}", std::to_string(ssl_error));
        return std::unexpected(TransportError::WriteError);
    }
    if (bytes_written != static_cast<int>(buffer.size())) {
        spdlog::error("SSL partial write: wrote {} of {} bytes", bytes_written, buffer.size());
        return std::unexpected(TransportError::WriteError);
    }
    spdlog::trace("successfully wrote {} bytes to SSL", bytes_written);
    return bytes_written;
}

bool TlsConnection::has_data() const {
    if (SSL_pending(ssl_) > 0) {
        return true;
    }

    constexpr int timeout_ms = 100;
    pollfd pfd = {client_fd_, POLLIN, 0};
    const int result = poll(&pfd, 1, timeout_ms);
    return result > 0 && (pfd.revents & POLLIN);
}

std::expected<std::string, ClientIpError> TlsConnection::client_ip() const {
    sockaddr_storage addr{};
    socklen_t len = sizeof(addr);

    if (getpeername(client_fd_, reinterpret_cast<sockaddr*>(&addr), &len) == -1) {
        spdlog::error("getpeername failed: {}", strerror(errno));
        return std::unexpected(ClientIpError::GetPeerNameFailed);
    }

    char ip_str[INET6_ADDRSTRLEN];
    if (addr.ss_family == AF_INET) {
        auto* s = reinterpret_cast<sockaddr_in*>(&addr);
        inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
    } else if (addr.ss_family == AF_INET6) {
        auto* s = reinterpret_cast<sockaddr_in6*>(&addr);
        inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
    } else {
        spdlog::error("unknown ss_family: {}", addr.ss_family);
        return std::unexpected(ClientIpError::UnknownIpFormat);
    }
    return std::string(ip_str);
}

}  // namespace ion
