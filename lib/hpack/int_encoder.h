#pragma once
#include <cstdint>
#include <vector>

namespace ion {

class IntegerEncoder {
   public:
    static std::vector<uint8_t> encode(uint32_t value, uint8_t prefix_bits);
};

}  // namespace ion
