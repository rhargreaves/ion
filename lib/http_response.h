#pragma once
#include <cstdint>

#include "headers/http_header.h"

namespace ion {

struct HttpResponse {
    uint16_t status_code;
    std::vector<uint8_t> body{};
    std::vector<HttpHeader> headers{};
};

}  // namespace ion
