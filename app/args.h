#pragma once
#include <spdlog/common.h>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>

static constexpr uint16_t DEFAULT_PORT = 8443;
static constexpr std::string_view DEFAULT_LOG_LEVEL = "info";

class Args {
   public:
    uint16_t port{DEFAULT_PORT};
    std::string log_level{DEFAULT_LOG_LEVEL};
    std::vector<std::string> static_map;
    std::string access_log_path{};

    static Args register_opts(CLI::App& app);
    static spdlog::level::level_enum parse_log_level(const std::string& level_str);
};
