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
    std::expected<ssize_t, TransportError> read(std::span<uint8_t> b) const override {
        if (input_.empty())
            return std::unexpected(TransportError::WantReadOrWrite);

        const auto count = std::min(buffer.size(), input_.size());
        std::copy_n(input_.begin(), count, buffer.begin());
        input_.erase(input_.begin(), input_.begin() + static_cast<std::ptrdiff_t>(count));
        return static_cast<ssize_t>(count);
    }
    std::expected<ssize_t, TransportError> write(std::span<const uint8_t> b) const override {
        output_.insert(output_.end(), b.begin(), b.end());
        return b.size();
    }
    std::expected<void, TransportError> handshake() const override {
        return {};
    }
    void graceful_shutdown() const override {}

   private:
    mutable std::vector<uint8_t> input_;
    mutable std::vector<uint8_t> output_;
};

}  // namespace ion::wasm
