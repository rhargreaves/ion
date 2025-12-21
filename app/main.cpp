#include <spdlog/spdlog.h>
#include <version.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>

#include "../lib/http2_server.h"
#include "access_log.h"
#include "args.h"
#include "signal_handler.h"
#include "spdlog/sinks/basic_file_sink.h"

void run_server(uint16_t port, const std::vector<std::string>& static_map, bool cleartext) {
    auto config = ion::ServerConfiguration::from_env();
    config.cleartext = cleartext;

    ion::Http2Server server{config};
    auto signal_handler = SignalHandler::setup(server);

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
    auto args = Args::register_opts(app);
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    spdlog::set_level(args.log_level_enum());
    spdlog::info("ion {} started ⚡️", ion::BUILD_VERSION);
    try {
        setup_access_logs(args.access_log_path);
        run_server(args.port, args.static_map, args.cleartext);
        spdlog::info("exiting");
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("fatal error: {}", e.what());
        return 1;
    }
}
