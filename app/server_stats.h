#pragma once
#include <chrono>
#include <map>

#include "router.h"

class ServerStats {
   public:
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    uint64_t total_requests{0};
    std::map<uint16_t, uint64_t> status_codes;
    int64_t total_duration_us{0};

    static ServerStats& instance() {
        static ServerStats s;
        return s;
    }

    static ion::Middleware middleware();
};
