#pragma once

#include "http2_server.h"
#include "wait_for_port.h"

class ServerTestFixture {
   public:
    explicit ServerTestFixture(ion::Http2Server& server, uint16_t port)
        : server_(server), server_thread_([&server, &port]() { server.run_server(port); }) {
        if (!wait_for_port(port)) {
            throw std::runtime_error("Server failed to start");
        }
    }

    ~ServerTestFixture() {
        server_.stop_server();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    ServerTestFixture(const ServerTestFixture&) = delete;
    ServerTestFixture& operator=(const ServerTestFixture&) = delete;

   private:
    ion::Http2Server& server_;
    std::thread server_thread_;
};
