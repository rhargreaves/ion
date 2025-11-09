#include <span>

class BitReader {
    std::span<const uint8_t> data_;
    size_t bit_pos_ = 0;

   public:
    BitReader(std::span<const uint8_t> bytes) : data_(bytes) {}

    bool read_bit() {
        if (bit_pos_ >= data_.size() * 8)
            return false;

        size_t byte_idx = bit_pos_ / 8;
        int bit_idx = 7 - (bit_pos_ % 8);
        ++bit_pos_;
        return (data_[byte_idx] >> bit_idx) & 1;
    }

    bool has_more() const {
        return bit_pos_ < data_.size() * 8;
    }
};
