#pragma once

#include "socket_fd.h"

namespace ion {

class TcpConnection {
   public:
    explicit TcpConnection(SocketFd&& client_fd);
    ~TcpConnection() = default;

    TcpConnection(TcpConnection&&) = default;
    TcpConnection& operator=(TcpConnection&&) = default;

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    void close();
    [[nodiscard]] int client_fd() const noexcept {
        return client_fd_;
    }

   private:
    SocketFd client_fd_;
};

}  // namespace ion
