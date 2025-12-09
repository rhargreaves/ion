#include <spdlog/spdlog.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <csignal>

#include "../lib/http2_server.h"
#include "args.h"

static std::atomic<ion::Http2Server*> g_server{nullptr};

static void stop_server() {
    if (ion::Http2Server* server = g_server.load()) {
        server->stop();
    }
}

void signal_handler(int) {
    spdlog::info("stopping server (signal)...");
    stop_server();
}

void sigpipe_handler(int) {
    char dummy;

    if (write(STDOUT_FILENO, &dummy, 0) < 0 && errno == EPIPE) {
        spdlog::error("stdout pipe broken, terminating");
        stop_server();
    }

    if (write(STDERR_FILENO, &dummy, 0) < 0 && errno == EPIPE) {
        spdlog::error("stderr pipe broken, terminating");
        stop_server();
    }

    spdlog::debug("SIGPIPE from network connection, ignoring");
}

void run_server(uint16_t port) {
    ion::Http2Server server{};
    g_server.store(&server);

    auto& router = server.router();

    router.add_route("/", "GET", [] { return ion::HttpResponse{.status_code = 200}; });

    router.add_route("/no_content", "GET", [] { return ion::HttpResponse{.status_code = 204}; });

    server.start(port);
}

int main(int argc, char* argv[]) {
    CLI::App app{"ion: the light-weight HTTP/2 server ⚡️"};
    uint16_t port{Args::DEFAULT_PORT};
    std::string log_level{Args::DEFAULT_LOG_LEVEL};
    Args::register_opts(app, port, log_level);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, sigpipe_handler);

    spdlog::set_level(Args::parse_log_level(log_level));
    spdlog::info("ion started ⚡️");
    try {
        run_server(port);
        spdlog::info("exiting");
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("fatal error: {}", e.what());
        return 1;
    }
}
