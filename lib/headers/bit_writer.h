#pragma once
#include <vector>

namespace ion {

class BitWriter {
   public:
    void write_bits(uint32_t value, uint8_t bit_count) {
        for (int i = bit_count - 1; i >= 0; --i) {  // MSB -> LSB
            bool bit = (value >> i) & 1;
            write_bit(bit);
        }
    }

    void write_bit(bool bit) {
        if (bit) {
            current_byte_ |= (1 << (7 - bits_in_current_));
        }
        bits_in_current_++;

        if (bits_in_current_ == 8) {
            buffer_.push_back(current_byte_);
            current_byte_ = 0;
            bits_in_current_ = 0;
        }
    }

    std::vector<uint8_t> finalize() {
        // Pad final byte with 1s (as per HPACK spec)
        if (bits_in_current_ > 0) {
            uint8_t padding_bits = 8 - bits_in_current_;
            for (uint8_t i = 0; i < padding_bits; ++i) {
                write_bit(true);
            }
        }
        return std::move(buffer_);
    }

    size_t bit_count() const {
        return buffer_.size() * 8 + bits_in_current_;
    }

   private:
    std::vector<uint8_t> buffer_;
    uint8_t current_byte_ = 0;
    uint8_t bits_in_current_ = 0;
};

}  // namespace ion
