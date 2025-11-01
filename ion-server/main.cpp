#include <csignal>

#include "http2_server.h"
#include "spdlog/spdlog.h"

static constexpr uint16_t SERVER_PORT = 8443;

static std::atomic<Http2Server*> g_server{nullptr};

void signal_handler(int) {
    if (Http2Server* server = g_server.load()) {
        spdlog::info("stopping server (signal)...");
        server->stop_server();
    }
}

int main() {
    spdlog::info("ion started ⚡️");
    spdlog::set_level(spdlog::level::debug);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        Http2Server server{};
        g_server.store(&server);

        server.run_server(SERVER_PORT);

        spdlog::info("exiting");
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("terminating: {}", e.what());
        return 1;
    }
}
