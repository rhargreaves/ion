#include "args.h"

#include <CLI/ExtraValidators.hpp>

#include "version.h"

Args Args::register_opts(CLI::App& app) {
    Args args{};

    app.add_option("--port,-p", args.port, "Port to listen on")
        ->default_val(DEFAULT_PORT)
        ->check(CLI::Range(1, 65535));

    app.add_option("--log-level,-l", args.log_level,
                   "Set logging level (trace, debug, info, warn, error, critical, off)")
        ->default_val(DEFAULT_LOG_LEVEL)
        ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error", "critical", "off"},
                              CLI::ignore_case));
    app.add_option("--static,-s", args.static_map,
                   "Map URL prefix to directory. Usage: --static /url/path ./fs/path")
        ->expected(2);

    app.add_option("--access-log", args.access_log_path)->default_val(std::nullopt);

    app.set_version_flag("-v,--version", std::string(ion::BUILD_VERSION));

    return args;
}

spdlog::level::level_enum Args::log_level_enum() const {
    static const std::unordered_map<std::string, spdlog::level::level_enum> level_map = {
        {"trace", spdlog::level::trace}, {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},   {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},   {"critical", spdlog::level::critical},
        {"off", spdlog::level::off}};

    if (auto it = level_map.find(log_level); it != level_map.end()) {
        return it->second;
    }
    throw CLI::ValidationError("Invalid log level: " + log_level);
}
