#include "server.h"

#include <cstddef>

#include "test_routes.h"

namespace ion::wasm {

Server::Server() {
    TestRoutes::add_test_routes(router_);
}

int Server::create_connection() {
    auto transport = std::make_unique<WasmTransport>();
    auto* raw = transport.get();
    auto conn = std::make_unique<Http2Connection>(std::move(transport), "wasm", router_);
    const int id = next_id_++;
    connections_.emplace(id, Connection{raw, std::move(conn)});
    return id;
}

void Server::feed(int id, std::span<const uint8_t> data) {
    if (const auto it = connections_.find(id); it != connections_.end()) {
        it->second.transport->feed(data);
    }
}

int Server::process(int id) {
    const auto it = connections_.find(id);
    if (it == connections_.end()) {
        return 1;
    }
    while (true) {
        switch (it->second.conn->process()) {
            case Http2ProcessResult::DiscardConnection:
                return 1;
            case Http2ProcessResult::WantRead:
                return 0;
            case Http2ProcessResult::WantWrite:
                break;
        }
    }
}

std::size_t Server::output_size(int id) const {
    const auto it = connections_.find(id);
    return it == connections_.end() ? 0 : it->second.transport->output().size();
}

std::size_t Server::output_read(int id, std::span<uint8_t> out) {
    const auto it = connections_.find(id);
    if (it == connections_.end()) {
        return 0;
    }
    auto& output = it->second.transport->output();
    const auto count = std::min(out.size(), output.size());
    std::copy_n(output.begin(), count, out.begin());
    output.erase(output.begin(), output.begin() + static_cast<std::ptrdiff_t>(count));
    return count;
}

void Server::close(int id) {
    connections_.erase(id);
}

}  // namespace ion::wasm
