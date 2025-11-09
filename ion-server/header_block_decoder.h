#pragma once
#include <span>
#include <vector>

struct HttpHeader {
    std::string name;
    std::string value;
};

struct StaticHttpHeader {
    std::string_view name;
    std::string_view value;

    [[nodiscard]] HttpHeader to_http_header() const {
        return HttpHeader{std::string(name), std::string(value)};
    }
};

class HeaderBlockDecoder {
   public:
    std::vector<HttpHeader> decode(std::span<const uint8_t> data);
};
