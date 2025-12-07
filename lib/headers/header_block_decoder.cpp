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

HeaderBlockDecoder::HeaderBlockDecoder(DynamicTable& dynamic_table)
    : dynamic_table_(dynamic_table) {
    populate_huffman_tree(huffman_tree_);
}

std::string HeaderBlockDecoder::read_length_and_string(ByteReader& reader) {
    if (!reader.has_bytes()) {
        throw std::runtime_error("Unexpected end of data while reading header length & string");
    }

    uint8_t length_byte = reader.read_byte();
    bool is_huffman = length_byte & 0x80;
    auto str_size = length_byte & 0x7F;

    if (!reader.has_bytes(str_size)) {
        throw std::runtime_error("Insufficient data for string");
    }
    return read_string(is_huffman, str_size, reader.read_bytes(str_size));
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

std::optional<HttpHeader> HeaderBlockDecoder::try_read_indexed_header(uint8_t index) {
    auto table_index = index - 1;
    if (table_index < STATIC_TABLE.size()) {
        // static lookup
        return STATIC_TABLE[table_index].to_http_header();
    }

    // dynamic lookup
    auto dynamic_index = table_index - STATIC_TABLE.size();
    if (dynamic_index >= dynamic_table_.size()) {
        spdlog::error("invalid dynamic table index ({})", index);
        return std::nullopt;
    }
    return dynamic_table_.get(dynamic_index);
}

std::optional<std::string> HeaderBlockDecoder::try_read_indexed_header_name(uint8_t index) {
    auto table_index = index - 1;
    if (table_index < STATIC_TABLE.size()) {
        // static lookup
        return std::string(STATIC_TABLE[table_index].name);
    }

    // dynamic lookup
    auto dynamic_index = table_index - STATIC_TABLE.size();
    if (dynamic_index >= dynamic_table_.size()) {
        spdlog::error("invalid dynamic table index ({})", index);
        return std::nullopt;
    }
    return dynamic_table_.get(dynamic_index).name;
}

std::optional<HttpHeader> HeaderBlockDecoder::try_decode_indexed_field(uint8_t first_byte) {
    auto index = static_cast<uint8_t>(first_byte & 0x7F);
    if (index < 1) {
        spdlog::error("invalid header index (<1)");
        return std::nullopt;
    }
    return try_read_indexed_header(index);
}

std::optional<HttpHeader> HeaderBlockDecoder::try_decode_literal_field(uint8_t index,
                                                                       ByteReader& reader,
                                                                       bool update_dynamic_table) {
    bool is_new_name = index == 0;
    const std::optional<std::string> name =
        is_new_name ? read_length_and_string(reader) : try_read_indexed_header_name(index);
    if (!name) {
        return std::nullopt;
    }

    auto value = read_length_and_string(reader);
    auto hdr = HttpHeader{name.value(), value};

    if (update_dynamic_table) {
        dynamic_table_.insert(hdr);
    }
    return hdr;
}

std::vector<HttpHeader> HeaderBlockDecoder::decode(std::span<const uint8_t> data) {
    auto hdrs = std::vector<HttpHeader>{};
    ByteReader reader(data);

    while (reader.has_bytes()) {
        uint8_t first_byte = reader.read_byte();
        if (first_byte & 0x80) {
            // indexed field (static or dynamic)
            if (auto hdr = try_decode_indexed_field(first_byte)) {
                hdrs.push_back(hdr.value());
            }
        } else if (first_byte & 0x40) {
            // literal header field
            auto index = static_cast<uint8_t>(first_byte & 0x3F);
            if (auto hdr = try_decode_literal_field(index, reader, true)) {
                hdrs.push_back(hdr.value());
            }
        } else if (first_byte > 0 && first_byte < 0x10) {
            // literal header field - indexed name, without indexing value
            auto index = static_cast<uint8_t>(first_byte & 0x0F);
            if (auto hdr = try_decode_literal_field(index, reader, false)) {
                hdrs.push_back(hdr.value());
            }
        } else if (first_byte == 0x00) {
            // literal header field - new name, without indexing value
            if (auto hdr = try_decode_literal_field(0, reader, false)) {
                hdrs.push_back(hdr.value());
            }
        } else {
            spdlog::error("invalid first byte in header representation: {}", first_byte);
        }
    }
    return hdrs;
}

}  // namespace ion
