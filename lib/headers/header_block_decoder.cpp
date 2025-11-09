#include "header_block_decoder.h"

#include <spdlog/spdlog.h>

#include <array>

#include "byte_reader.h"
#include "header_static_table.h"
#include "huffman_codes.h"
#include "huffman_tree.h"

namespace ion {

static void populate_huffman_tree(HuffmanTree& tree) {
    for (uint16_t i = 0; i < static_cast<uint16_t>(HUFFMAN_CODES.size()); i++) {
        auto code = HUFFMAN_CODES[i];
        tree.insert_symbol(static_cast<uint8_t>(i), code.lsb_aligned_code, code.code_len);
    }
}

HeaderBlockDecoder::HeaderBlockDecoder() {
    populate_huffman_tree(huffman_tree_);
}

std::string HeaderBlockDecoder::read_string(bool is_huffman, ssize_t size,
                                            std::span<const uint8_t> data) {
    if (is_huffman) {
        auto raw_bytes = huffman_tree_.decode(data.subspan(0, size));
        return {raw_bytes.begin(), raw_bytes.end()};
    }
    auto raw_data = data.subspan(0, size);
    return {raw_data.begin(), raw_data.end()};
}

std::vector<HttpHeader> HeaderBlockDecoder::decode(std::span<const uint8_t> data) {
    auto hdrs = std::vector<HttpHeader>{};
    ByteReader reader(data);

    while (reader.has_bytes()) {
        uint8_t first_byte = reader.read_byte();

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
            // literal header field
            auto index = static_cast<uint8_t>(first_byte & 0x3F);
            if (index == 0) {
                // - new name
                // check bytes
                if (!reader.has_bytes()) {
                    spdlog::error("Unexpected end of data while reading name length");
                    break;
                }

                // get name
                uint8_t name_length_byte = reader.read_byte();
                auto name_size = name_length_byte & 0x7F;
                bool name_is_huffman = name_length_byte & 0x80;
                auto name = read_string(name_is_huffman, name_size, reader.read_bytes(name_size));

                // check bytes
                if (!reader.has_bytes()) {
                    spdlog::error("Unexpected end of data while reading value length");
                    break;
                }

                // get value
                uint8_t value_length_byte = reader.read_byte();
                auto value_size = value_length_byte & 0x7F;
                bool value_is_huffman = value_length_byte & 0x80;

                if (!reader.has_bytes(value_size)) {
                    spdlog::error("Insufficient data for value field");
                    break;
                }
                auto value_bytes = reader.read_bytes(value_size);
                auto val = read_string(value_is_huffman, value_size, value_bytes);

                auto hdr = HttpHeader{name, val};
                hdrs.push_back(hdr);
                dynamic_table_.push_back(hdr);
            } else {
                // - incremental index
                // get name
                auto name = std::string(STATIC_TABLE[index - 1].name);

                // get value
                if (!reader.has_bytes()) {
                    spdlog::error("Unexpected end of data while reading value length");
                    break;
                }

                uint8_t value_length_byte = reader.read_byte();
                auto value_size = value_length_byte & 0x7F;
                bool value_is_huffman = value_length_byte & 0x80;

                if (!reader.has_bytes(value_size)) {
                    spdlog::error("Insufficient data for value field");
                    break;
                }
                auto value_bytes = reader.read_bytes(value_size);
                auto val = read_string(value_is_huffman, value_size, value_bytes);

                auto hdr = HttpHeader{name, val};
                hdrs.push_back(hdr);
                dynamic_table_.push_back(hdr);
            }
        }
    }
    return hdrs;
}

}  // namespace ion
