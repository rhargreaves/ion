#include "args.h"

#include <CLI/ExtraValidators.hpp>

void Args::register_opts(CLI::App& app, uint16_t& port, std::string& log_level,
                         std::vector<std::string>& static_dir) {
    app.add_option("--port,-p", port, "Port to listen on")
        ->default_val(DEFAULT_PORT)
        ->check(CLI::Range(1, 65535));

    app.add_option("--log-level,-l", log_level,
                   "Set logging level (trace, debug, info, warn, error, critical, off)")
        ->default_val(DEFAULT_LOG_LEVEL)
        ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error", "critical", "off"},
                              CLI::ignore_case));

    app.add_option("--static,-s", static_dir,
                   "Map URL prefix to directory. Usage: --static /url/path ./fs/path")
        ->expected(2);
}

spdlog::level::level_enum Args::parse_log_level(const std::string& level_str) {
    static const std::unordered_map<std::string, spdlog::level::level_enum> level_map = {
        {"trace", spdlog::level::trace}, {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},   {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},   {"critical", spdlog::level::critical},
        {"off", spdlog::level::off}};

    if (auto it = level_map.find(level_str); it != level_map.end()) {
        return it->second;
    }
    throw CLI::ValidationError("Invalid log level: " + level_str);
}
