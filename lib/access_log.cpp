#include "access_log.h"

#include <spdlog/spdlog.h>

#include <format>

#include "system_clock.h"

namespace ion {

void AccessLog::log_request(std::vector<HttpHeader>& req_headers, uint16_t status_code,
                            size_t content_length, const std::string& client_ip) {
    auto access_log = spdlog::get(ACCESS_LOGGER_NAME);
    if (!access_log) {
        return;
    }

    std::string method{"-"};
    std::string path{"-"};
    std::string user_agent{"-"};
    std::string referrer{"-"};

    for (const auto& [name, value] : req_headers) {
        if (name == ":method") {
            method = value;
        }
        if (name == ":path") {
            path = value;
        }
        if (name == "user-agent") {
            user_agent = value;
        }
        if (name == "referer") {
            referrer = value;
        }
    }
    const auto entry =
        std::format(R"({} - - {} "{} {} HTTP/2" {} {} "{}" "{}")",
                    (client_ip.empty() ? "-" : client_ip), SystemClock::clf_timestamp(), method,
                    path, status_code, content_length, referrer, user_agent);
    access_log->info(entry);
}

}  // namespace ion
