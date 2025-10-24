#include "tls_conn.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

#include "tls_conn_except.h"

TlsConnection::TlsConnection(TcpConnection& tcp_conn) : tcp_conn_(tcp_conn) {}

TlsConnection::~TlsConnection() {
    if (ssl_) {
        BIO_flush(SSL_get_wbio(ssl_));
        // Bidirectional shutdown
        if (const int ret = SSL_shutdown(ssl_); ret == 0) {
            SSL_shutdown(ssl_);
        }
        SSL_free(ssl_);
        ssl_ = nullptr;
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
        std::cout << "ALPN negotiated: ";
        std::cout.write(reinterpret_cast<const char*>(*out), *outlen) << std::endl;
        return SSL_TLSEXT_ERR_OK;
    }

    *outlen = supported_protos[0];
    *out = supported_protos.data() + 1;
    std::cout << "ALPN negotiation failed, using default: ";
    std::cout.write(reinterpret_cast<const char*>(*out), *outlen) << std::endl;
    return SSL_TLSEXT_ERR_OK;
}

void TlsConnection::handshake(const std::filesystem::path& cert_path,
                              const std::filesystem::path& key_path) {
    if (!exists(cert_path)) {
        throw std::runtime_error("Certificate file not found");
    }

    if (!exists(key_path)) {
        throw std::runtime_error("Private key file not found");
    }

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        throw std::runtime_error("Failed to create SSL context");
    }

    if (SSL_CTX_use_certificate_file(ctx, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("Failed to load certificate file");
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("Failed to load private key file");
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("Private key does not match certificate");
    }

    SSL_CTX_set_alpn_select_cb(ctx, alpn_callback, nullptr);

    SSL_CTX_set_keylog_callback(ctx, [](const SSL*, const char* line) {
        if (auto keylog_file = std::getenv("SSLKEYLOGFILE")) {
            std::ofstream file(keylog_file, std::ios::app);
            if (file.is_open()) {
                file << line << '\n';
            }
        }
    });

    ssl_ = SSL_new(ctx);
    if (!ssl_) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("Failed to create SSL object");
    }

    if (SSL_set_fd(ssl_, tcp_conn_.client_fd()) != 1) {
        throw std::runtime_error("Failed to set SSL file descriptor");
    }

    constexpr int timeout_ms = 100;
    constexpr int max_retries = 100;

    for (int attempt = 0; attempt < max_retries; ++attempt) {
        const int result = SSL_accept(ssl_);
        if (result == 1) {
            return;
        }
        switch (const int ssl_error = SSL_get_error(ssl_, result)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE: {
                pollfd pfd = {tcp_conn_.client_fd(), 0, 0};
                pfd.events = (ssl_error == SSL_ERROR_WANT_READ) ? POLLIN : POLLOUT;
                const int poll_result = poll(&pfd, 1, timeout_ms);
                if (poll_result > 0 && (pfd.revents & pfd.events)) {
                    continue;  // Socket ready, try again
                }
                if (poll_result == 0) {
                    continue;  // Timeout, try again
                }
                if (poll_result < 0) {
                    throw std::system_error(errno, std::system_category(), "poll during handshake");
                }
                break;
            }
            default: {
                char err_buf[256];
                ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
                throw std::runtime_error("SSL handshake failed: " + std::string(err_buf));
            }
        }
    }
}

void TlsConnection::print_debug_to_stderr() { ERR_print_errors_fp(stderr); }

ssize_t TlsConnection::read(std::span<uint8_t> buffer) const {
    const auto bytes_read = SSL_read(ssl_, buffer.data(), static_cast<int>(buffer.size()));
    if (bytes_read < 0) {
        const int ssl_error = SSL_get_error(ssl_, bytes_read);
        switch (ssl_error) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                return 0;  // no data available right now
            case SSL_ERROR_ZERO_RETURN:
                throw TlsConnectionClosed(TlsConnectionClosed::Reason::CLEAN_SHUTDOWN);

            default:
                throw std::runtime_error("SSL read error: " + std::to_string(ssl_error));
        }
    }
    return bytes_read;
}

ssize_t TlsConnection::write(std::span<const uint8_t> buffer) const {
    const auto bytes_written = SSL_write(ssl_, buffer.data(), static_cast<int>(buffer.size()));
    if (bytes_written < 0) {
        const int ssl_error = SSL_get_error(ssl_, bytes_written);
        throw std::runtime_error("SSL write error: " + std::to_string(ssl_error));
    }
    return bytes_written;
}

bool TlsConnection::has_data() const {
    if (SSL_pending(ssl_) > 0) {
        return true;
    }

    constexpr int timeout_ms = 100;
    pollfd pfd = {tcp_conn_.client_fd(), POLLIN, 0};
    const int result = poll(&pfd, 1, timeout_ms);
    return result > 0 && (pfd.revents & POLLIN);
}
