#include <spdlog/spdlog.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <csignal>

#include "../lib/http2_server.h"
#include "access_log.h"
#include "args.h"
#include "spdlog/sinks/basic_file_sink.h"

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

void run_server(uint16_t port, const std::vector<std::string>& static_map) {
    ion::Http2Server server{};
    g_server.store(&server);

    auto& router = server.router();

    router.add_route("/_tests/ok", "GET", [] { return ion::HttpResponse{.status_code = 200}; });
    router.add_route("/_tests/no_content", "GET",
                     [] { return ion::HttpResponse{.status_code = 204}; });

    if (static_map.size() == 2) {
        auto sth = std::make_unique<ion::StaticFileHandler>(static_map[0], static_map[1]);
        router.add_static_handler(std::move(sth));
    }

    server.start(port);
}

void setup_access_logs(const std::string& log_path) {
    if (log_path.empty()) {
        return;
    }

    try {
        auto logger = spdlog::basic_logger_mt(ion::AccessLog::ACCESS_LOGGER_NAME, log_path);
        logger->set_pattern("%v");
        logger->flush_on(spdlog::level::info);
        spdlog::info("writing access logs to {}", log_path);
    } catch (const spdlog::spdlog_ex& e) {
        spdlog::error("failed to setup access logs: {}", e.what());
    }
}

int main(int argc, char* argv[]) {
    CLI::App app{"ion: the light-weight HTTP/2 server ⚡️"};

    auto opts = Args::register_opts(app);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, sigpipe_handler);

    spdlog::set_level(Args::parse_log_level(opts.log_level));
    spdlog::info("ion started ⚡️");
    try {
        setup_access_logs(opts.access_log_path);
        run_server(opts.port, opts.static_map);
        spdlog::info("exiting");
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("fatal error: {}", e.what());
        return 1;
    }
}
