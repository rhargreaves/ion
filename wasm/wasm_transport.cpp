#include "wasm_transport.h"

#include <expected>

#include "transports/transport.h"

namespace ion::wasm {

std::expected<ssize_t, TransportError> WasmTransport::read(std::span<uint8_t> buffer) const {
    if (input_.empty())
        return std::unexpected(TransportError::WantReadOrWrite);

    const auto count = std::min(buffer.size(), input_.size());
    std::copy_n(input_.begin(), count, buffer.begin());
    input_.erase(input_.begin(), input_.begin() + static_cast<std::ptrdiff_t>(count));
    return static_cast<ssize_t>(count);
}

std::expected<ssize_t, TransportError> WasmTransport::write(std::span<const uint8_t> buffer) const {
    output_.insert(output_.end(), buffer.begin(), buffer.end());
    return buffer.size();
}

std::expected<void, TransportError> WasmTransport::handshake() const {
    return {};
}

void WasmTransport::graceful_shutdown() const {}

void WasmTransport::feed(std::span<const uint8_t> data) const {
    input_.insert(input_.end(), data.begin(), data.end());
}

std::vector<uint8_t>& WasmTransport::output() const {
    return output_;
}

}  // namespace ion::wasm
