#pragma once
#include <openssl/ssl.h>

#include <expected>
#include <filesystem>
#include <span>

#include "socket_fd.h"
#include "tls_context.h"
#include "transport.h"

namespace ion {

class TlsTransport : public Transport {
   public:
    explicit TlsTransport(SocketFd&& client_fd, const TlsContext& tls_context);
    ~TlsTransport() override;

    TlsTransport(TlsTransport&& other) noexcept;
    TlsTransport& operator=(TlsTransport&& other) noexcept;

    TlsTransport(const TlsTransport&) = delete;
    TlsTransport& operator=(const TlsTransport&) = delete;

    static void print_debug_to_stderr();
    std::expected<ssize_t, TransportError> read(std::span<uint8_t> buffer) const override;
    std::expected<ssize_t, TransportError> write(std::span<const uint8_t> buffer) const override;
    [[nodiscard]] std::expected<void, TransportError> handshake() const override;
    void graceful_shutdown() const override;

   private:
    SocketFd client_fd_;
    SSL* ssl_ = nullptr;
};

}  // namespace ion
