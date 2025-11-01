#include <CLI/CLI.hpp>
#include <csignal>

#include "http2_server.h"
#include "spdlog/spdlog.h"

static constexpr uint16_t DEFAULT_PORT = 8443;

static std::atomic<Http2Server*> g_server{nullptr};

void signal_handler(int) {
    if (Http2Server* server = g_server.load()) {
        spdlog::info("stopping server (signal)...");
        server->stop_server();
    }
}

int main(int argc, char* argv[]) {
    CLI::App app{"ion: the light-weight HTTP/2 server ⚡️"};

    uint16_t port = DEFAULT_PORT;
    app.add_option("--port,-p", port, "Port to listen on")
        ->default_val(DEFAULT_PORT)
        ->check(CLI::Range(1, 65535));

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    spdlog::info("ion started ⚡️");
    spdlog::set_level(spdlog::level::debug);
    try {
        Http2Server server{};
        g_server.store(&server);

        server.run_server(port);

        spdlog::info("exiting");
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("terminating: {}", e.what());
        return 1;
    }
}
