#include "tls_transport.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <unistd.h>

#include <fstream>

namespace ion {

TlsTransport::TlsTransport(TlsTransport&& other) noexcept
    : client_fd_(std::move(other.client_fd_)), ssl_(std::exchange(other.ssl_, nullptr)) {}

TlsTransport& TlsTransport::operator=(TlsTransport&& other) noexcept {
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

TlsTransport::TlsTransport(SocketFd&& client_fd, const TlsContext& tls_context)
    : client_fd_(std::move(client_fd)) {
    ssl_ = tls_context.create_ssl();

    if (SSL_set_fd(ssl_, client_fd_) != 1) {
        throw std::runtime_error("failed to set SSL file descriptor");
    }
}

bool TlsTransport::handshake() const {
    constexpr int timeout_ms = 100;
    constexpr int max_retries = 50;

    for (int attempt = 0; attempt < max_retries; attempt++) {
        const int result = SSL_accept(ssl_);
        if (result == 1) {
            return true;
        }
        switch (const int ssl_error = SSL_get_error(ssl_, result)) {
            case SSL_ERROR_ZERO_RETURN:
                spdlog::debug(
                    "TLS connection closed by peer before handshake (likely health check)");
                return false;
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
            case SSL_ERROR_SSL: {
                const unsigned long err = ERR_get_error();
                if (ERR_GET_REASON(err) == SSL_R_UNEXPECTED_EOF_WHILE_READING) {
                    spdlog::info("client disconnected during handshake (EOF)");
                    return false;
                }
                [[fallthrough]];
            }
            default: {
                char err_buf[256];
                ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
                spdlog::error("TLS handshake failed: ({}) {}", ssl_error, err_buf);
                return false;
            }
        }
    }

    spdlog::error("SSL handshake timed out after {} attempts", max_retries);
    return false;
}

TlsTransport::~TlsTransport() {
    if (ssl_) {
        spdlog::debug("freeing OpenSSL");
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
}

void TlsTransport::graceful_shutdown() const {
    BIO_flush(SSL_get_wbio(ssl_));
    // Bidirectional shutdown
    spdlog::debug("shutting down SSL (client notify)");
    if (const int ret = SSL_shutdown(ssl_); ret == 0) {
        spdlog::debug("shutting down SSL (force)");
        SSL_shutdown(ssl_);
    }
}

void TlsTransport::print_debug_to_stderr() {
    ERR_print_errors_fp(stderr);
}

std::expected<ssize_t, TransportError> TlsTransport::read(std::span<uint8_t> buffer) const {
    spdlog::trace("reading from SSL");
    const auto bytes_read = SSL_read(ssl_, buffer.data(), static_cast<int>(buffer.size()));
    if (bytes_read <= 0) {
        switch (const int ssl_error = SSL_get_error(ssl_, bytes_read)) {
            case SSL_ERROR_WANT_READ:
                spdlog::trace("SSL want read");
                return std::unexpected(TransportError::WantReadOrWrite);
            case SSL_ERROR_WANT_WRITE:
                spdlog::trace("SSL want write");
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

std::expected<ssize_t, TransportError> TlsTransport::write(std::span<const uint8_t> buffer) const {
    const auto bytes_written = SSL_write(ssl_, buffer.data(), static_cast<int>(buffer.size()));
    if (bytes_written < 0) {
        switch (const int ssl_error = SSL_get_error(ssl_, bytes_written)) {
            case SSL_ERROR_WANT_READ:
                spdlog::trace("SSL want read");
                return std::unexpected(TransportError::WantReadOrWrite);
            case SSL_ERROR_WANT_WRITE:
                spdlog::trace("SSL want write");
                return std::unexpected(TransportError::WantReadOrWrite);
            case SSL_ERROR_ZERO_RETURN:
                spdlog::debug("TLS connection closed");
                return std::unexpected(TransportError::ConnectionClosed);
            case SSL_ERROR_SSL:
                spdlog::error("TLS protocol error (SSL_ERROR_SSL)");
                return std::unexpected(TransportError::ProtocolError);
            default:
                spdlog::error("TLS protocol error (SSL_ERROR={})", ssl_error);
                return std::unexpected(TransportError::ProtocolError);
        }
    }
    if (bytes_written != static_cast<int>(buffer.size())) {
        spdlog::error("SSL partial write: wrote {} of {} bytes", bytes_written, buffer.size());
        return std::unexpected(TransportError::WriteError);
    }
    spdlog::trace("successfully wrote {} bytes to SSL", bytes_written);
    return bytes_written;
}

}  // namespace ion
