#include "header_block_decoder.h"

#include <array>

// https://datatracker.ietf.org/doc/html/rfc7541#appendix-A
constexpr std::array<HttpHeader, 61> STATIC_TABLE = {  //
    {{":authority", ""},
     {":method", "GET"},
     {":method", "POST"},
     {":path", "/"},
     {":path", "/index.html"},
     {":scheme", "http"},
     {":scheme", "https"},
     {":status", "200"},
     {":status", "204"},
     {":status", "206"},
     {":status", "304"},
     {":status", "400"},
     {":status", "404"},
     {":status", "500"},
     {"accept-charset", ""},
     {"accept-encoding", "gzip, deflate"},
     {"accept-language", ""},
     {"accept-ranges", ""},
     {"accept", ""},
     {"access-control-allow-origin", ""},
     {"age", ""},
     {"allow", ""},
     {"authorization", ""},
     {"cache-control", ""},
     {"content-disposition", ""},
     {"content-encoding", ""},
     {"content-language", ""},
     {"content-length", ""},
     {"content-location", ""},
     {"content-range", ""},
     {"content-type", ""},
     {"cookie", ""},
     {"date", ""},
     {"etag", ""},
     {"expect", ""},
     {"expires", ""},
     {"from", ""},
     {"host", ""},
     {"if-match", ""},
     {"if-modified-since", ""},
     {"if-none-match", ""},
     {"if-range", ""},
     {"if-unmodified-since", ""},
     {"last-modified", ""},
     {"link", ""},
     {"location", ""},
     {"max-forwards", ""},
     {"proxy-authenticate", ""},
     {"proxy-authorization", ""},
     {"range", ""},
     {"referer", ""},
     {"refresh", ""},
     {"retry-after", ""},
     {"server", ""},
     {"set-cookie", ""},
     {"strict-transport-security", ""},
     {"transfer-encoding", ""},
     {"user-agent", ""},
     {"vary", ""},
     {"via", ""},
     {"www-authenticate", ""}}};

std::vector<HttpHeader> HeaderBlockDecoder::decode(std::span<const uint8_t> data) {
    auto hdrs = std::vector<HttpHeader>{};
    for (auto& chunk : data) {
        if (chunk & 0x80) {
            // indexed
            auto index = static_cast<uint8_t>(chunk & 0x7F);
            if (index < 1 || index > STATIC_TABLE.size()) {
                continue;
            }
            auto hdr = STATIC_TABLE[index - 1];
            hdrs.push_back(hdr);
        }
    }
    return hdrs;
}
