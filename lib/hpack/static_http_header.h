#pragma once
#include <string>

#include "http_header.h"

namespace ion {

struct StaticHttpHeader {
    std::string_view name;
    std::string_view value;

    [[nodiscard]] HttpHeader to_http_header() const {
        return HttpHeader{std::string(name), std::string(value)};
    }
};

}  // namespace ion
