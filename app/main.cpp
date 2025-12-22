#include <spdlog/spdlog.h>
#include <version.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>

#include "../lib/http2_server.h"
#include "access_log.h"
#include "args.h"
#include "signal_handler.h"
#include "spdlog/sinks/basic_file_sink.h"

#ifdef __linux__
#include <sys/prctl.h>
#endif

void run_server(const Args& args) {
    ion::Http2Server server{args.to_server_config()};
    auto signal_handler = SignalHandler::setup(server);

    auto& router = server.router();
    router.add_route("/_tests/ok", "GET", [] { return ion::HttpResponse{.status_code = 200}; });
    router.add_route("/_tests/no_content", "GET",
                     [] { return ion::HttpResponse{.status_code = 204}; });
    router.add_route("/_tests/no_new_privs", "GET", [] {
        bool no_new_privs = false;
#ifdef __linux__
        int result = prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0);
        if (result == 1)
            no_new_privs = true;
#endif

        if (no_new_privs) {
            return ion::HttpResponse{.status_code = 200};
        } else {
            return ion::HttpResponse{.status_code = 500};
        }
    });

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

int main(int argc, char* argv[]) {
#ifdef __linux__
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
        spdlog::critical("failed to set PR_SET_NO_NEW_PRIVS: {}", std::strerror(errno));
        return EXIT_FAILURE;
    }
    spdlog::debug("security: PR_SET_NO_NEW_PRIVS enabled");
#endif

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
        run_server(args);
        spdlog::info("exiting");
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        spdlog::critical("fatal error: {}", e.what());
        return EXIT_FAILURE;
    }
}
