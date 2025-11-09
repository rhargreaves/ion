#include "huffman_tree.h"

#include <format>

#include "bit_reader.h"

void HuffmanTree::insert_symbol(int16_t symbol, uint32_t lsb_aligned_code, uint8_t code_len) {
    if (code_len == 0 || code_len > 31) {
        throw std::runtime_error("Invalid code length: " + std::to_string(code_len));
    }

    HuffmanNode* current = root_.get();
    for (int i = code_len - 1; i >= 0; --i) {
        bool bit = (lsb_aligned_code >> i) & 1;
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

    if (current->symbol != -1) {
        throw std::runtime_error("Duplicate symbol: " + std::to_string(symbol));
    }
    current->symbol = symbol;
}

std::vector<int16_t> HuffmanTree::decode(std::span<const uint8_t> data) {
    std::vector<int16_t> result;

    BitReader br{data};
    const HuffmanNode* current = root_.get();
    size_t bits_processed = 0;
    while (br.has_more()) {
        if (!br.read_bit()) {
            current = current->left.get();
        } else {
            current = current->right.get();
        }
        bits_processed++;

        if (!current) {
            throw std::runtime_error(
                std::format("Invalid Huffman code: (bit pos = {})", bits_processed));
        }

        if (current->is_terminal()) {
            result.push_back(current->symbol);
            current = root_.get();
        }
    }

    return result;
}
