#include "tcp_transport.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ion {

TcpTransport::TcpTransport(SocketFd&& client_fd) : client_fd_(std::move(client_fd)) {}

TcpTransport::~TcpTransport() = default;

TcpTransport::TcpTransport(TcpTransport&& other) noexcept
    : client_fd_(std::move(other.client_fd_)) {}

TcpTransport& TcpTransport::operator=(TcpTransport&& other) noexcept {
    if (this != &other) {
        client_fd_ = std::move(other.client_fd_);
    }
    return *this;
}

std::expected<ssize_t, TransportError> TcpTransport::read(std::span<uint8_t> buffer) const {
    spdlog::trace("reading from TCP socket");
    const auto bytes_read = ::read(client_fd_, buffer.data(), buffer.size());
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            spdlog::trace("TCP want read");
            return std::unexpected(TransportError::WantReadOrWrite);
        }
        spdlog::error("TCP read error: {}", strerror(errno));
        return std::unexpected(TransportError::OtherError);
    }
    if (bytes_read == 0) {
        spdlog::debug("TCP connection closed");
        return std::unexpected(TransportError::ConnectionClosed);
    }
    spdlog::trace("successfully read {} bytes from TCP socket", bytes_read);
    return bytes_read;
}

std::expected<ssize_t, TransportError> TcpTransport::write(std::span<const uint8_t> buffer) const {
    const auto bytes_written = ::write(client_fd_, buffer.data(), buffer.size());
    if (bytes_written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            spdlog::trace("TCP want write");
            return std::unexpected(TransportError::WantReadOrWrite);
        }
        spdlog::error("TCP write error: {}", strerror(errno));
        return std::unexpected(TransportError::WriteError);
    }
    if (bytes_written != static_cast<ssize_t>(buffer.size())) {
        spdlog::error("TCP partial write: wrote {} of {} bytes", bytes_written, buffer.size());
        // For non-blocking sockets, partial writes can happen
        // TODO: Fix this!
        return std::unexpected(TransportError::WriteError);
    }
    spdlog::trace("successfully wrote {} bytes to TCP socket", bytes_written);
    return bytes_written;
}

void TcpTransport::graceful_shutdown() const {
    spdlog::debug("shutting down TCP (SHUT_WR)");
    if (shutdown(client_fd_, SHUT_WR) == -1) {
        spdlog::error("shutdown failed: {}", strerror(errno));
    }
}

std::expected<std::string, ClientIpError> TcpTransport::client_ip() const {
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
