#pragma once
#include <optional>

#include "socket_fd.h"
#include "tcp_conn.h"

namespace ion {

class TcpListener {
   public:
    explicit TcpListener(uint16_t port);
    ~TcpListener() = default;

    void listen() const;
    [[nodiscard]] std::optional<TcpConnection> try_accept();
    void close();

   private:
    SocketFd server_fd_;

    static void set_nonblocking_socket(const SocketFd& socket_fd);
    void set_reusable_addr();
    void bind_socket(uint16_t port);
};

}  // namespace ion
