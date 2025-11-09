#pragma once
#include <memory>
#include <optional>
#include <span>
#include <vector>

struct HuffmanNode {
    std::unique_ptr<HuffmanNode> left;
    std::unique_ptr<HuffmanNode> right;
    std::optional<uint8_t> symbol;

    bool is_terminal() const {
        return symbol.has_value();
    }
};

class HuffmanTree {
   public:
    HuffmanTree() = default;
    ~HuffmanTree() = default;

    void insert_symbol(int16_t symbol, uint32_t lsb_aligned_code, uint8_t code_len);
    std::vector<uint8_t> decode(std::span<const uint8_t> data);

   private:
    std::unique_ptr<HuffmanNode> root_ = std::make_unique<HuffmanNode>();
};
