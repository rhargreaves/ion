#include "status_endpoint.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <string>

#include <nlohmann/json.hpp>

#include "http_response.h"
#include "server_stats.h"
#include "version.h"

void StatusEndpoint::add_status_endpoint(ion::Router& router) {
    router.add_middleware(ServerStats::middleware());

    auto& stats = ServerStats::instance();
    spdlog::info("instance id: {}", stats.server_id);

    router.add_route("/_ion/status", "GET", [&stats](const auto&) {
        const auto now = std::chrono::steady_clock::now();
        const auto uptime_sec =
            std::chrono::duration_cast<std::chrono::seconds>(now - stats.start_time).count();

        const uint64_t total = stats.total_requests;
        const double avg_latency_ms =
            total > 0 ? (double)stats.total_duration_us / total / 1000.0 : 0.0;

        nlohmann::json status{
            {"version", std::string(ion::BUILD_VERSION)},
            {"instance_id", stats.server_id},
            {"uptime_seconds", uptime_sec},
            {"requests", stats.total_requests},
            {"avg_latency_ms", avg_latency_ms},
            {"status_codes", nlohmann::json::object()},
        };

        for (const auto& [code, count] : stats.status_codes) {
            status["status_codes"][std::to_string(code)] = count;
        }

        const auto body_str = status.dump();
        return ion::HttpResponse{
            .status_code = 200,
            .body = std::vector<uint8_t>{body_str.begin(), body_str.end()},
            .headers = {{"content-type", "application/json"},
                        {"cache-control", "no-store"}},
        };
    });
}