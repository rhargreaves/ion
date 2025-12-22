#include <spdlog/spdlog.h>
#include <version.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>

#include "../lib/http2_server.h"
#include "access_log.h"
#include "args.h"
#include "proc_ctrl.h"
#include "signal_handler.h"
#include "spdlog/sinks/basic_file_sink.h"

void run_server(const Args& args) {
    ion::Http2Server server{args.to_server_config()};
    auto signal_handler = SignalHandler::setup(server);

    auto& router = server.router();
    if (args.under_test) {
        router.add_route("/_tests/ok", "GET", [] { return ion::HttpResponse{.status_code = 200}; });
        router.add_route("/_tests/no_content", "GET",
                         [] { return ion::HttpResponse{.status_code = 204}; });
        router.add_route("/_tests/no_new_privs", "GET", [] {
            if (ProcessControl::check_no_new_privs()) {
                return ion::HttpResponse{.status_code = 200};
            }
            return ion::HttpResponse{.status_code = 500};
        });
    }

    if (args.static_map.size() == 2) {
        auto sth = std::make_unique<ion::StaticFileHandler>(args.static_map[0], args.static_map[1]);
        router.add_static_handler(std::move(sth));
    }

    server.start(args.port);
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

void enable_no_new_privs_on_linux() {
#ifdef __linux__
    if (ProcessControl::enable_no_new_privs()) {
        spdlog::warn("failed to set PR_SET_NO_NEW_PRIVS: {}", std::strerror(errno));
    }
    spdlog::debug("PR_SET_NO_NEW_PRIVS enabled");
#endif
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
        enable_no_new_privs_on_linux();
        setup_access_logs(args.access_log_path);
        run_server(args);
        spdlog::info("exiting");
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        spdlog::critical("fatal error: {}", e.what());
        return EXIT_FAILURE;
    }
}
