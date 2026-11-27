#pragma once
#include <algorithm>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

#include "transports/transport.h"

namespace ion::wasm {

class WasmTransport final : public Transport {
   public:
    std::expected<ssize_t, TransportError> read(std::span<uint8_t> buffer) const override;
    std::expected<ssize_t, TransportError> write(std::span<const uint8_t> buffer) const override;
    std::expected<void, TransportError> handshake() const override;
    void graceful_shutdown() const override;
    void feed(std::span<const uint8_t> data) const;
    std::vector<uint8_t>& output() const;

   private:
    mutable std::vector<uint8_t> input_;
    mutable std::vector<uint8_t> output_;
};

}  // namespace ion::wasm
