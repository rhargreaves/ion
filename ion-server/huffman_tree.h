#pragma once
#include <span>
#include <vector>

struct HuffmanNode {
    HuffmanNode* left;
    HuffmanNode* right;
    int16_t symbol = -1;
    bool is_terminal() const {
        return symbol != -1;
    }
};

class HuffmanTree {
   public:
    ~HuffmanTree();

    void insert_symbol(int16_t symbol, uint32_t lsb_aligned_code, uint8_t code_len);
    std::vector<int16_t> decode(std::span<const uint8_t> data, size_t bit_len);

   private:
    static void delete_tree(HuffmanNode* node);

    HuffmanNode* root_ = new HuffmanNode();
};
