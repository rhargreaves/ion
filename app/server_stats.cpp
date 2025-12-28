#include "server_stats.h"

ion::Middleware ServerStats::middleware() {
    return [](auto next) {
        return [next](const auto& req) {
            auto& stats = instance();
            stats.total_requests++;
            auto start = std::chrono::steady_clock::now();

            auto res = next(req);

            auto end = std::chrono::steady_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            stats.total_duration_us += duration;
            stats.status_codes[res.status_code]++;
            return res;
        };
    };
}
