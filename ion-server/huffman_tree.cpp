#include "huffman_tree.h"

#include "bit_reader.h"

void HuffmanTree::insert_symbol(int16_t symbol, uint32_t lsb_aligned_code, uint8_t code_len) {
    HuffmanNode* current = root_.get();
    while (code_len--) {
        bool bit = (lsb_aligned_code >> code_len) & 1;
        if (!bit) {
            if (!current->left) {
                current->left = std::make_unique<HuffmanNode>();
            }
            current = current->left.get();
        } else {
            if (!current->right) {
                current->right = std::make_unique<HuffmanNode>();
            }
            current = current->right.get();
        }
    }
    current->symbol = symbol;
}

std::vector<int16_t> HuffmanTree::decode(std::span<const uint8_t> data, size_t bit_len) {
    std::vector<int16_t> result;

    BitReader br{data};
    HuffmanNode* current = root_.get();
    while (br.has_more() && bit_len--) {
        bool current_bit = br.read_bit();
        if (!current_bit) {
            current = current->left.get();
        } else {
            current = current->right.get();
        }

        if (!current) {
            throw std::runtime_error("Invalid Huffman code");
        }

        if (current->is_terminal()) {
            result.push_back(current->symbol);
            current = root_.get();
        }
    }

    return result;
}
