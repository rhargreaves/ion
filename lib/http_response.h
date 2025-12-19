#pragma once
#include <cstdint>
#include <vector>

#include "hpack/http_header.h"

namespace ion {

struct HttpResponse {
    uint16_t status_code;
    std::vector<uint8_t> body{};
    std::vector<HttpHeader> headers{};
};

}  // namespace ion
