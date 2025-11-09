#include "header_block_decoder.h"

#include <spdlog/spdlog.h>

#include <array>

#include "huffman_codes.h"
#include "huffman_tree.h"

// https://datatracker.ietf.org/doc/html/rfc7541#appendix-A
constexpr std::array<StaticHttpHeader, 61> STATIC_TABLE = {  //
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
    HuffmanTree huffmanTree{};
    for (uint16_t i = 0; i < static_cast<uint16_t>(HUFFMAN_CODES.size()); i++) {
        auto code = HUFFMAN_CODES[i];
        huffmanTree.insert_symbol(i, code.lsb_aligned_code, code.code_len);
    }

    auto hdrs = std::vector<HttpHeader>{};
    for (size_t i = 0; i < data.size();) {
        uint8_t first_byte = data[i];

        if (first_byte & 0x80) {
            // indexed
            auto index = static_cast<uint8_t>(first_byte & 0x7F);
            if (index < 1 || index > STATIC_TABLE.size()) {
                i++;
                continue;
            }
            auto hdr = STATIC_TABLE[index - 1];
            hdrs.push_back(hdr.to_http_header());
            i++;
        } else if (first_byte & 0x40) {
            // literal header field with incremental index
            auto index = static_cast<uint8_t>(first_byte & 0x3F);
            i++;

            bool is_huffman = data[i] & 0x80;
            auto value_size = data[i] & 0x7F;
            i++;

            if (is_huffman) {
                auto hdr_name = STATIC_TABLE[index - 1].name;
                auto hdr_value = huffmanTree.decode(data.subspan(i, value_size), value_size * 8);
                std::string result;
                result.reserve(hdr_value.size());

                for (int16_t symbol : hdr_value) {
                    if (symbol >= 0 && symbol <= 255) {  // Valid ASCII/UTF-8
                        result.push_back(static_cast<char>(symbol));
                    }
                }

                auto hdr = HttpHeader{std::string(hdr_name), result};
                hdrs.push_back(hdr);
                i += value_size;
            } else {
                spdlog::error("non-huffman strings not supported yet");
                i += value_size;
            }
        }
    }
    return hdrs;
}
