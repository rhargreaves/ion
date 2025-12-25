#include "huffman_tree.h"

#include <format>

#include "bit_reader.h"
#include "spdlog/spdlog.h"

namespace ion {

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

    if (current->symbol) {
        throw std::runtime_error("Duplicate symbol: " + std::to_string(symbol));
    }
    current->symbol = symbol;
}

std::expected<std::vector<uint8_t>, HuffmanDecodeError> HuffmanTree::decode(
    std::span<const uint8_t> data) {
    std::vector<uint8_t> result;

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
            while (br.has_more()) {
                if (!br.read_bit()) {
                    spdlog::error("remaining unmatched Huffman code is not padding: (bit pos = {})",
                                  bits_processed);
                    return std::unexpected{HuffmanDecodeError::InvalidCode};
                }
                bits_processed++;
            }
            return result;
        }

        if (current->is_terminal()) {
            result.push_back(*current->symbol);
            current = root_.get();
        }
    }

    return result;
}

}  // namespace ion
