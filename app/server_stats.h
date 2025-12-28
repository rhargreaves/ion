#pragma once
#include <chrono>
#include <map>

#include "router.h"

class ServerStats {
   public:
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    uint64_t total_requests{0};
    std::map<uint16_t, uint64_t> status_codes{};
    int64_t total_duration_us{0};
    std::string server_id = generate_id();

    ServerStats() = default;
    ~ServerStats() = default;

    ServerStats(const ServerStats&) = delete;
    ServerStats& operator=(const ServerStats&) = delete;

    static ServerStats& instance() {
        static ServerStats s;
        return s;
    }

    static ion::Middleware middleware();

   private:
    static std::string generate_id();
};
