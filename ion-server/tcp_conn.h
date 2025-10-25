#pragma once
#include <filesystem>

#include "socket_fd.h"

class TcpConnection {
   public:
    explicit TcpConnection(uint16_t port);
    ~TcpConnection() = default;

    void listen() const;
    bool try_accept();
    void close();
    [[nodiscard]] int server_fd() const noexcept { return server_fd_; }
    [[nodiscard]] int client_fd() const noexcept { return client_fd_; }

   private:
    SocketFd server_fd_;
    SocketFd client_fd_;

    void set_nonblocking_socket();
    void set_reusable_addr();
    void bind_socket(uint16_t port);
};
