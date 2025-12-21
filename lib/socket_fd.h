#pragma once

#include <expected>
#include <string>

namespace ion {

enum class ClientIpError { GetPeerNameFailed, UnknownIpFormat };

class SocketFd {
   public:
    explicit SocketFd(int fd = -1);
    ~SocketFd();

    SocketFd(const SocketFd&) = delete;
    SocketFd& operator=(const SocketFd&) = delete;

    SocketFd(SocketFd&& other) noexcept;
    SocketFd& operator=(SocketFd&& other) noexcept;

    std::expected<std::string, ClientIpError> client_ip() const;
    operator int() const noexcept;
    void close() noexcept;

   private:
    int fd_;
};

}  // namespace ion
