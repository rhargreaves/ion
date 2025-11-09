#include "header_block_decoder.h"

#include <spdlog/spdlog.h>

#include <array>

#include "header_static_table.h"
#include "huffman_codes.h"
#include "huffman_tree.h"

std::vector<HttpHeader> HeaderBlockDecoder::decode(std::span<const uint8_t> data) {
    HuffmanTree huffmanTree{};
    for (uint16_t i = 0; i < static_cast<uint16_t>(HUFFMAN_CODES.size()); i++) {
        auto code = HUFFMAN_CODES[i];
        huffmanTree.insert_symbol(i, code.lsb_aligned_code, code.code_len);
    }

    auto hdrs = std::vector<HttpHeader>{};
    for (size_t i = 0; i < data.size(); i++) {
        uint8_t first_byte = data[i];

        if (first_byte & 0x80) {
            // indexed
            auto index = static_cast<uint8_t>(first_byte & 0x7F);
            if (index < 1) {
                spdlog::error("invalid header index (<1)");
                continue;
            }
            auto table_index = index - 1;
            if (table_index < STATIC_TABLE.size()) {
                // static lookup
                auto hdr = STATIC_TABLE[table_index];
                hdrs.push_back(hdr.to_http_header());
            } else {
                // dynamic lookup
                auto dynamic_index = table_index - STATIC_TABLE.size();
                if (dynamic_index >= dynamic_table_.size()) {
                    spdlog::error("invalid dynamic table index ({})", index);
                    continue;
                }
                auto hdr = dynamic_table_[dynamic_index];
                hdrs.push_back(hdr);
            }

        } else if (first_byte & 0x40) {
            // literal header field with incremental index
            auto index = static_cast<uint8_t>(first_byte & 0x3F);
            i++;

            bool is_huffman = data[i] & 0x80;
            auto value_size = data[i] & 0x7F;
            i++;

            if (is_huffman) {
                auto hdr_name = STATIC_TABLE[index - 1].name;
                auto decoded = huffmanTree.decode(data.subspan(i, value_size));
                const std::string hdr_value(decoded.begin(), decoded.end());

                auto hdr = HttpHeader{std::string(hdr_name), hdr_value};
                hdrs.push_back(hdr);
                dynamic_table_.push_back(hdr);
            } else {
                spdlog::error("non-huffman strings not supported yet");
            }
            i += value_size;
        }
    }
    return hdrs;
}
