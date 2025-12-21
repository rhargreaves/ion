#pragma once
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <expected>

namespace ion {

enum class ClientIpError { GetPeerNameFailed, UnknownIpFormat };

class SocketFd {
   public:
    explicit SocketFd(int fd = -1) : fd_(fd) {}

    SocketFd(const SocketFd&) = delete;
    SocketFd& operator=(const SocketFd&) = delete;

    SocketFd(SocketFd&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }
    SocketFd& operator=(SocketFd&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    ~SocketFd() {
        close();
    }

    std::expected<std::string, ClientIpError> client_ip() const;

    operator int() const noexcept {
        return fd_;
    }

    void close() noexcept {
        if (fd_ >= 0) {
            spdlog::trace("closing socket fd {}", fd_);
            ::close(fd_);
            fd_ = -1;
        }
    }

   private:
    int fd_;
};

}  // namespace ion
