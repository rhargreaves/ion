#include "tcp_listener.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/err.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <limits>
#include <system_error>

namespace ion {

inline int to_poll_timeout_ms(std::chrono::milliseconds timeout) {
    if (timeout.count() < 0) {
        return -1;  // wait forever
    }
    if (timeout.count() > std::numeric_limits<int>::max()) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(timeout.count());
}

void TcpListener::set_nonblocking_socket(const SocketFd& socket_fd) {
    const int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::system_error(errno, std::system_category(), "fcntl F_GETFL");
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::system_error(errno, std::system_category(), "fcntl F_SETFL");
    }
}

void TcpListener::set_reusable_addr() {
    constexpr int enable_opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &enable_opt, sizeof(enable_opt))) {
        throw std::system_error(errno, std::system_category(), "setsockopt SO_REUSEADDR");
    }
}

void TcpListener::bind_socket(uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::system_error(errno, std::system_category(), "bind");
    }
}

TcpListener::TcpListener(uint16_t port) {
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::system_error(errno, std::system_category(), "socket");
    }
    server_fd_ = SocketFd(server_fd);
    set_nonblocking_socket(server_fd_);
    set_reusable_addr();
    bind_socket(port);
}

void TcpListener::listen() {
    if (::listen(server_fd_, 1) < 0) {
        throw std::system_error(errno, std::system_category(), "listen");
    }
}

int TcpListener::raw_fd() const {
    return server_fd_;
}

std::optional<SocketFd> TcpListener::try_accept() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    const int fd = ::accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (fd >= 0) {
        auto client_fd = SocketFd(fd);
        set_nonblocking_socket(client_fd);  // required for Linux. macOS inherits from server_fd!

        // Disable Nagle's algorithm for low-latency HTTP/2 framing
        constexpr int enable = 1;
        if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) < 0) {
            spdlog::warn("failed to set TCP_NODELAY on client fd: {}", strerror(errno));
        }

        return client_fd;
    }

    if (errno == EAGAIN || errno == EINTR) {
        return std::nullopt;
    }

    throw std::system_error(errno, std::system_category(), "accept");
}

void TcpListener::close() {
    server_fd_.close();
}

}  // namespace ion
