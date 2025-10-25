#include "tcp_conn.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <system_error>

void TcpConnection::set_nonblocking_socket() {
    const int flags = fcntl(server_fd_, F_GETFL, 0);
    if (flags == -1) {
        throw std::system_error(errno, std::system_category(), "fcntl F_GETFL");
    }
    if (fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::system_error(errno, std::system_category(), "fcntl F_SETFL");
    }
}

void TcpConnection::set_reusable_addr() {
    constexpr int enable_opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &enable_opt, sizeof(enable_opt))) {
        throw std::system_error(errno, std::system_category(), "setsockopt SO_REUSEADDR");
    }
}

void TcpConnection::bind_socket(uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::system_error(errno, std::system_category(), "bind");
    }
}

TcpConnection::TcpConnection(uint16_t port) {
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::system_error(errno, std::system_category(), "socket");
    }
    server_fd_ = SocketFd(server_fd);
    set_nonblocking_socket();
    set_reusable_addr();
    bind_socket(port);
}

void TcpConnection::listen() const {
    if (::listen(server_fd_, 1) < 0) {
        throw std::system_error(errno, std::system_category(), "listen");
    }
}

bool TcpConnection::try_accept() {
    constexpr int timeout_ms = 100;
    pollfd pfd = {server_fd_, POLLIN, 0};
    const int result = poll(&pfd, 1, timeout_ms);
    if (result < 0) {
        throw std::system_error(errno, std::system_category(), "poll");
    }

    if (pfd.revents & POLLIN) {  // connection available
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int tmp_client_fd =
            ::accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (tmp_client_fd >= 0) {
            client_fd_ = SocketFd(tmp_client_fd);
            return true;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;
        }
        throw std::system_error(errno, std::system_category(), "accept");
    }
    return false;
}

void TcpConnection::close() {
    client_fd_.close();
    server_fd_.close();
}
