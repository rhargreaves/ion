#include "huffman_tree.h"

#include "bit_reader.h"

void HuffmanTree::insert_symbol(int16_t symbol, uint32_t lsb_aligned_code, uint8_t code_len) {
    HuffmanNode* current = root_;
    while (code_len--) {
        bool bit = (lsb_aligned_code >> code_len) & 1;
        if (!bit) {
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

void HuffmanTree::delete_tree(HuffmanNode* node) {
    if (!node)
        return;
    delete_tree(node->left);
    delete_tree(node->right);
    delete node;
}

HuffmanTree::~HuffmanTree() {
    delete_tree(root_);
}
