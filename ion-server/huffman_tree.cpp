#include "huffman_tree.h"

#include "bit_reader.h"

void HuffmanTree::insert_symbol(int16_t symbol, uint32_t code, uint8_t bit_len) {
    auto code_span =
        std::array{static_cast<uint8_t>(code >> 24 & 0xFF), static_cast<uint8_t>(code >> 16 & 0xFF),
                   static_cast<uint8_t>(code >> 8 & 0xFF), static_cast<uint8_t>(code & 0xFF)};
    BitReader br{code_span};

    HuffmanNode* current = root_;
    while (br.has_more() && bit_len--) {
        bool current_bit = br.read_bit();
        if (!current_bit) {
            if (!current->left) {
                current->left = new HuffmanNode();
            }
            current = current->left;
        } else {
            if (!current->right) {
                current->right = new HuffmanNode();
            }
            current = current->right;
        }
    }

    current->symbol = symbol;
}

std::vector<int16_t> HuffmanTree::decode(std::span<const uint8_t> data, size_t bit_len) {
    std::vector<int16_t> result;

    BitReader br{data};
    HuffmanNode* current = root_;
    while (br.has_more() && bit_len--) {
        bool current_bit = br.read_bit();
        if (!current_bit) {
            current = current->left;
        } else {
            current = current->right;
        }

        if (!current) {
            throw std::runtime_error("Invalid Huffman code");
        }

        if (current->is_terminal()) {
            result.push_back(current->symbol);
            current = root_;
        }
    }

    return result;
}
