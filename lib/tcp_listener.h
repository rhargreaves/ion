#pragma once
#include <chrono>
#include <optional>

#include "socket_fd.h"

namespace ion {

class TcpListener {
   public:
    explicit TcpListener(uint16_t port);
    ~TcpListener() = default;

    void listen() const;
    [[nodiscard]] std::optional<SocketFd> try_accept(std::chrono::milliseconds timeout);
    void close();

   private:
    SocketFd server_fd_;

    static void set_nonblocking_socket(const SocketFd& socket_fd);
    void set_reusable_addr();
    void bind_socket(uint16_t port);
};

}  // namespace ion
