#include "header_block_decoder.h"

#include <spdlog/spdlog.h>

#include <array>

#include "byte_reader.h"
#include "header_field.h"
#include "header_static_table.h"
#include "huffman_codes.h"
#include "huffman_tree.h"
#include "int_decoder.h"

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

std::expected<std::string, FrameError> HeaderBlockDecoder::read_length_and_string(
    ByteReader& reader) {
    const auto length_byte_res = reader.read_byte();
    if (!length_byte_res) {
        spdlog::error("Unexpected end of data while reading header length & string");
        return std::unexpected(FrameError::ProtocolError);
    }

    const bool is_huffman = *length_byte_res & 0x80;
    const auto str_size = *length_byte_res & 0x7F;

    const auto str_span_res = reader.read_bytes(str_size);
    if (!str_span_res) {
        spdlog::error("Insufficient data for string");
        return std::unexpected(FrameError::ProtocolError);
    }
    return read_string(is_huffman, str_size, *str_span_res);
}

std::expected<std::string, FrameError> HeaderBlockDecoder::read_string(
    bool is_huffman, size_t size, std::span<const uint8_t> data) {
    if (is_huffman) {
        auto bytes_res = huffman_tree_.decode(data.subspan(0, size));
        if (!bytes_res) {
            return std::unexpected{FrameError::ProtocolError};
        }
        return std::string{bytes_res->begin(), bytes_res->end()};
    }
    auto raw_data = data.subspan(0, size);
    return std::string{raw_data.begin(), raw_data.end()};
}

std::expected<HttpHeader, FrameError> HeaderBlockDecoder::read_indexed_header(size_t index) {
    auto table_index = index - 1;
    if (table_index < STATIC_TABLE.size()) {
        // static lookup
        auto hdr = STATIC_TABLE[table_index];
        spdlog::trace("read indexed header from static table (idx: {}, name: {}, val: {})",
                      table_index, hdr.name, hdr.value);
        return hdr.to_http_header();
    }

    // dynamic lookup
    auto dynamic_index = table_index - STATIC_TABLE.size();
    if (dynamic_index >= dynamic_table_.count()) {
        spdlog::error("invalid dynamic table index lookup for header (idx: {}, sz: {})", index,
                      dynamic_table_.count());
        return std::unexpected(FrameError::ProtocolError);
    }
    auto hdr = dynamic_table_.get(dynamic_index);
    spdlog::trace("read indexed header from dynamic table (idx: {}, name: {}, val: {})",
                  dynamic_index, hdr.name, hdr.value);
    return hdr;
}

std::expected<std::string, FrameError> HeaderBlockDecoder::read_indexed_header_name(size_t index) {
    auto table_index = index - 1;
    if (table_index < STATIC_TABLE.size()) {
        // static lookup
        auto name = STATIC_TABLE[table_index].name;
        spdlog::trace("read indexed header name from static table (idx: {}, name: {})", table_index,
                      name);
        return std::string(name);
    }

    // dynamic lookup
    auto dynamic_index = table_index - STATIC_TABLE.size();
    if (dynamic_index >= dynamic_table_.count()) {
        spdlog::error("invalid dynamic table index lookup for header name (idx: {}, sz: {})", index,
                      dynamic_table_.count());
        return std::unexpected(FrameError::ProtocolError);
    }
    auto name = dynamic_table_.get(dynamic_index).name;
    spdlog::trace("read indexed header name from dynamic table (idx: {}, name: {})", dynamic_index,
                  name);
    return name;
}

std::expected<HttpHeader, FrameError> HeaderBlockDecoder::decode_indexed_field(ByteReader& reader) {
    const auto index = IntegerDecoder::decode(reader, 7);
    if (!index.has_value()) {
        spdlog::error("failed to decode indexed header index (not enough bytes)");
    }

    if (*index < 1) {
        spdlog::error("invalid header index (<1)");
        return std::unexpected(FrameError::ProtocolError);
    }
    return read_indexed_header(*index);
}

std::expected<HttpHeader, FrameError> HeaderBlockDecoder::decode_literal_field(
    uint8_t idx_prefix_bits, ByteReader& reader) {
    const auto index = IntegerDecoder::decode(reader, idx_prefix_bits);
    if (!index.has_value()) {
        spdlog::error("failed to decode literal field index (not enough bytes)");
    }

    bool is_new_name = *index == 0;
    spdlog::trace("decoding literal field with index: {}, new name: {}", *index, is_new_name);
    const auto name =
        is_new_name ? read_length_and_string(reader) : read_indexed_header_name(*index);
    if (!name) {
        return std::unexpected(FrameError::ProtocolError);
    }

    auto value = read_length_and_string(reader);
    if (!value) {
        return std::unexpected(FrameError::ProtocolError);
    }
    spdlog::trace("decoded header: name: {}, value: {}", name.value(), value.value());
    return HttpHeader{name.value(), *value};
}

std::expected<void, FrameError> HeaderBlockDecoder::decode_dynamic_table_size_update(
    ByteReader& reader) {
    const auto new_sz = IntegerDecoder::decode(reader, 5);
    if (!new_sz) {
        spdlog::error("failed to decode dynamic table size update (not enough bytes)");
        return std::unexpected(FrameError::ProtocolError);
    }
    dynamic_table_.set_max_table_size(*new_sz);
    spdlog::trace("dynamic table size updated to: {}", *new_sz);
    return {};
}

std::expected<std::vector<HttpHeader>, FrameError> HeaderBlockDecoder::decode(
    std::span<const uint8_t> data) {
    auto hdrs = std::vector<HttpHeader>{};
    ByteReader reader(data);

    while (reader.has_bytes()) {
        uint8_t first_byte = *reader.peek_byte();
        auto type = HeaderField::from_byte(first_byte);
        spdlog::trace("header type: {}, byte: 0x{:02X}", HeaderField::to_string(type), first_byte);
        switch (type) {
            case HeaderFieldType::Indexed: {
                if (auto hdr = decode_indexed_field(reader)) {
                    hdrs.push_back(*hdr);
                } else {
                    return std::unexpected(hdr.error());
                }
                break;
            }
            case HeaderFieldType::LiteralIncremental: {
                if (auto hdr = decode_literal_field(6, reader)) {
                    dynamic_table_.insert(hdr.value());
                    hdrs.push_back(hdr.value());
                } else {
                    return std::unexpected(hdr.error());
                }
                break;
            }
            case HeaderFieldType::LiteralNoIndex:
            case HeaderFieldType::LiteralNeverIndex: {
                if (auto hdr = decode_literal_field(4, reader)) {
                    hdrs.push_back(hdr.value());
                } else {
                    return std::unexpected(hdr.error());
                }
                break;
            }
            case HeaderFieldType::SizeUpdate: {
                if (auto res = decode_dynamic_table_size_update(reader); !res) {
                    return std::unexpected(res.error());
                }
                break;
            }
            case HeaderFieldType::Invalid: {
                spdlog::error("invalid first byte in header representation: {}", first_byte);
                return std::unexpected(FrameError::ProtocolError);
            }
        }
    }
    return hdrs;
}

}  // namespace ion
