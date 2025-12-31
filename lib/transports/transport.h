#pragma once
#include <unistd.h>

#include <expected>
#include <span>

namespace ion {

enum class TransportError {
    WantReadOrWrite,
    WriteError,
    ConnectionClosed,
    ProtocolError,
    OtherError
};

class Transport {
   public:
    virtual ~Transport() = default;

    virtual std::expected<ssize_t, TransportError> read(std::span<uint8_t> buffer) const = 0;
    virtual std::expected<ssize_t, TransportError> write(std::span<const uint8_t> buffer) const = 0;
    virtual void graceful_shutdown() const = 0;
    [[nodiscard]] virtual std::expected<void, TransportError> handshake() const = 0;
};

}  // namespace ion
