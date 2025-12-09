#pragma once

#include "http2_server.h"
#include "wait_for_port.h"

class TestServerRunner {
   public:
    explicit TestServerRunner(ion::Http2Server& server, uint16_t port)
        : server_(server), server_thread_([&server, &port]() { server.start(port); }) {
        if (!wait_for_port(port)) {
            throw std::runtime_error("Server failed to start");
        }
    }

    ~TestServerRunner() {
        server_.stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    TestServerRunner(const TestServerRunner&) = delete;
    TestServerRunner& operator=(const TestServerRunner&) = delete;

   private:
    ion::Http2Server& server_;
    std::thread server_thread_;
};
