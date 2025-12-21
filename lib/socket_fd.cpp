#include "socket_fd.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "transport.h"

namespace ion {

std::expected<std::string, ClientIpError> SocketFd::client_ip() const {
    sockaddr_storage addr{};
    socklen_t len = sizeof(addr);

    if (getpeername(fd_, reinterpret_cast<sockaddr*>(&addr), &len) == -1) {
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
