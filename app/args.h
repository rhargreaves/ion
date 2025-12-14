#pragma once
#include <spdlog/common.h>

#include <CLI/App.hpp>

class Args {
   public:
    static constexpr uint16_t DEFAULT_PORT = 8443;
    static constexpr std::string_view DEFAULT_LOG_LEVEL = "info";

    static void register_opts(CLI::App& app, uint16_t& port, std::string& log_level);
    static spdlog::level::level_enum parse_log_level(const std::string& level_str);
};
