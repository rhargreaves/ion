

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

enum class ClientIpError { GetPeerNameFailed, UnknownIpFormat };

class Transport {
   public:
    virtual ~Transport() = default;

    virtual std::expected<ssize_t, TransportError> read(std::span<uint8_t> buffer) const = 0;
    virtual std::expected<ssize_t, TransportError> write(std::span<const uint8_t> buffer) const = 0;
    [[nodiscard]] virtual bool has_data() const = 0;
    virtual void graceful_shutdown() const = 0;
    virtual std::expected<std::string, ClientIpError> client_ip() const = 0;
};

}  // namespace ion
