#include "int_encoder.h"

#include <stdexcept>

namespace ion {

std::vector<uint8_t> IntegerEncoder::encode(uint32_t value, uint8_t prefix_bits) {
    if (prefix_bits == 0 || prefix_bits > 8) {
        throw std::out_of_range("invalid prefix bits: " + std::to_string(prefix_bits));
    }

    if (value < (1 << prefix_bits) - 1) {
        return {static_cast<uint8_t>(value)};
    }

    std::vector<uint8_t> result{};
    result.push_back(static_cast<uint8_t>((1 << prefix_bits) - 1));
    value -= (1 << prefix_bits) - 1;

    while (value >= 0x80) {
        result.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
        value >>= 7;
    }

    result.push_back(static_cast<uint8_t>(value));
    return result;
}

}  // namespace ion
