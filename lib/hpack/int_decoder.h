#pragma once
#include <expected>

#include "byte_reader.h"

namespace ion {

enum class IntegerDecodeError { NotEnoughBytes };

class IntegerDecoder {
   public:
    static std::expected<uint32_t, IntegerDecodeError> decode(ByteReader& reader,
                                                              uint8_t prefix_bits);
};

}  // namespace ion
