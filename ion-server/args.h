#pragma once
#include "CLI/App.hpp"
#include "spdlog/common.h"

class Args {
   public:
    static constexpr uint16_t DEFAULT_PORT = 8443;
    static constexpr std::string_view DEFAULT_LOG_LEVEL = "debug";

    static void register_opts(CLI::App& app, uint16_t& port, std::string& log_level);
    static spdlog::level::level_enum parse_log_level(const std::string& level_str);
};
