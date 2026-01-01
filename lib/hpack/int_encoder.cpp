#include "int_encoder.h"

namespace ion {

std::vector<uint8_t> IntegerEncoder::encode(uint32_t value, uint8_t prefix_bits) {
    std::vector<uint8_t> result{};

    // The prefix size, N, is always between 1 and 8 bits.  An integer
    // starting at an octet boundary will have an 8-bit prefix.
    //
    // Pseudocode to represent an integer I is as follows:
    //
    // if I < 2^N - 1, encode I on N bits
    // else
    //     encode (2^N - 1) on N bits
    //     I = I - (2^N - 1)
    //     while I >= 128
    //          encode (I % 128 + 128) on 8 bits
    //          I = I / 128
    //     encode I on 8 bits

    if (value < (1 << prefix_bits) - 1) {
        return {static_cast<uint8_t>(value)};
    }

    result.push_back(static_cast<uint8_t>((1 << prefix_bits) - 1));
    value -= (1 << prefix_bits) - 1;
    while (value >= 128) {
        result.push_back(static_cast<uint8_t>(value % 128 + 128));
        value /= 128;
    }
    result.push_back(static_cast<uint8_t>(value));
    return result;
}

}  // namespace ion
