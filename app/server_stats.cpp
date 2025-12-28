#include "server_stats.h"

#include <iomanip>
#include <random>

std::string ServerStats::generate_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << dis(gen);
    return ss.str();
}

ion::Middleware ServerStats::middleware() {
    return [](auto next) {
        return [next](const auto& req) {
            auto& stats = instance();
            stats.total_requests++;
            const auto start = std::chrono::steady_clock::now();

            auto res = next(req);

            const auto end = std::chrono::steady_clock::now();
            const auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            stats.total_duration_us += duration;
            stats.status_codes[res.status_code]++;
            return res;
        };
    };
}
