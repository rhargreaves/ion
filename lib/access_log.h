#pragma once
#include <vector>

#include "hpack/http_header.h"

namespace ion {

class AccessLog {
   public:
    static constexpr std::string ACCESS_LOGGER_NAME = "access";

    static void log_request(std::vector<HttpHeader>& req_headers, uint16_t status_code,
                            size_t content_length, const std::string& client_ip);
};

}  // namespace ion
