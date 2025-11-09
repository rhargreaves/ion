#pragma once
#include <span>
#include <vector>

#include "huffman_tree.h"

namespace ion {

struct HttpHeader {
    std::string name;
    std::string value;
};

class HeaderBlockDecoder {
   public:
    HeaderBlockDecoder();
    std::vector<HttpHeader> decode(std::span<const uint8_t> data);

   private:
    std::vector<HttpHeader> dynamic_table_;
    HuffmanTree huffman_tree_{};

    std::string read_string(bool is_huffman, ssize_t size, std::span<const uint8_t> data);
};

}  // namespace ion
