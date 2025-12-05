#include "tcp_conn.h"

#include <algorithm>

namespace ion {

TcpConnection::TcpConnection(SocketFd&& client_fd) : client_fd_(std::move(client_fd)) {}

void TcpConnection::close() {
    client_fd_.close();
}

}  // namespace ion
