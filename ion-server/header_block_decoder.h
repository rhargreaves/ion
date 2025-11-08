#pragma once
#include <span>
#include <vector>

struct HttpHeader {
    std::string_view name;
    std::string_view value;

    constexpr HttpHeader(const char* n, const char* v) : name(n), value(v) {}
    HttpHeader(const std::string& n, const std::string& v) : name(n), value(v) {}
};

class HeaderBlockDecoder {
   public:
    std::vector<HttpHeader> decode(std::span<const uint8_t> data);
};
