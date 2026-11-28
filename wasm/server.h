#pragma once
#include <spdlog/spdlog.h>

#include <memory>

#include "http2_conn.h"
#include "wasm_transport.h"

namespace ion::wasm {

class Server {
   public:
    Server();
    int create_connection();
    void feed(int id, std::span<const uint8_t> data);
    [[nodiscard]] int process(int id);
    [[nodiscard]] std::size_t output_size(int id) const;
    [[nodiscard]] std::size_t output_read(int id, std::span<uint8_t> out);
    void close(int id);

   private:
    struct Connection {
        WasmTransport* transport;
        std::unique_ptr<Http2Connection> conn;
    };

    Router router_{};
    std::unordered_map<int, Connection> connections_;
    int next_id_ = 1;
};

}  // namespace ion::wasm
