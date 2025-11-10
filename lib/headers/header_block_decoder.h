#pragma once
#include <span>
#include <vector>

#include "byte_reader.h"
#include "http_header.h"
#include "huffman_tree.h"

namespace ion {

class HeaderBlockDecoder {
   public:
    HeaderBlockDecoder();
    std::vector<HttpHeader> decode(std::span<const uint8_t> data);

   private:
    std::vector<HttpHeader> dynamic_table_;
    HuffmanTree huffman_tree_{};

    std::string read_string(bool is_huffman, ssize_t size, std::span<const uint8_t> data);
    std::string read_length_and_string(ByteReader& reader);
    std::optional<HttpHeader> try_decode_literal_field(uint8_t first_byte, ByteReader& reader);
    std::optional<HttpHeader> try_decode_indexed_field(uint8_t first_byte);
    std::optional<std::string> try_read_indexed_header_name(uint8_t index);
    std::optional<HttpHeader> try_read_indexed_header(uint8_t index);
};

}  // namespace ion
