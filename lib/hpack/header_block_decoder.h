#pragma once
#include <expected>
#include <span>
#include <vector>

#include "byte_reader.h"
#include "dynamic_table.h"
#include "frame_error.h"
#include "http_header.h"
#include "huffman_tree.h"

namespace ion {

class HeaderBlockDecoder {
   public:
    explicit HeaderBlockDecoder(DynamicTable& dynamic_table);

    std::expected<std::vector<HttpHeader>, FrameError> decode(std::span<const uint8_t> data);

   private:
    DynamicTable& dynamic_table_;
    HuffmanTree huffman_tree_{};

    std::expected<std::string, FrameError> read_string(bool is_huffman, size_t size,
                                                       std::span<const uint8_t> data);
    std::expected<std::string, FrameError> read_length_and_string(ByteReader& reader);
    std::expected<HttpHeader, FrameError> decode_literal_field(uint8_t idx_prefix_bits,
                                                               ByteReader& reader);
    std::expected<HttpHeader, FrameError> decode_indexed_field(ByteReader& reader);
    std::expected<std::string, FrameError> read_indexed_header_name(size_t index);
    std::expected<HttpHeader, FrameError> read_indexed_header(size_t index);
};

}  // namespace ion
