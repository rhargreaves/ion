#pragma once
#include <cstdint>

#include "socket_fd.h"
#include "transport.h"

namespace ion {

class TcpTransport : public Transport {
   public:
    explicit TcpTransport(SocketFd&& client_fd);
    ~TcpTransport() override;

    TcpTransport(TcpTransport&& other) noexcept;
    TcpTransport& operator=(TcpTransport&& other) noexcept;

    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;

    std::expected<ssize_t, TransportError> read(std::span<uint8_t> buffer) const override;
    std::expected<ssize_t, TransportError> write(std::span<const uint8_t> buffer) const override;
    void graceful_shutdown() const override;
    [[nodiscard]] std::expected<void, TransportError> handshake() const override;

   private:
    SocketFd client_fd_;
};

}  // namespace ion
