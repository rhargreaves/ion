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

    app.add_flag("--cleartext", args.cleartext,
                 "Disables TLS and handles requests in HTTP/2 cleartext (h2c)");

    app.add_option("--tls-cert-path", args.cert_path, "Path to certificate file for TLS")
        ->envname("ION_TLS_CERT_PATH");

    app.add_option("--tls-key-path", args.key_path, "Path to private key file for TLS")
        ->envname("ION_TLS_KEY_PATH");

    app.add_option("--custom-404-file-path", args.status_404_file_path, "Path to custom 404 page");

    app.add_flag("--under-test", args.under_test,
                 "Adds routes used for internal testing. Do not enable in production");

    app.add_flag("--status-page", args.status_page,
                 "Adds page (/_ion/status) displaying server status");

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

ion::ServerConfiguration Args::to_server_config() const {
    auto config = ion::ServerConfiguration{};
    config.cleartext = cleartext;
    if (!cert_path.empty()) {
        config.cert_path = cert_path;
    }
    if (!key_path.empty()) {
        config.key_path = key_path;
    }
    config.key_path = key_path;
    return config;
}
