#pragma once
#include <cstdint>

namespace ion {

enum class HeaderFieldType { Indexed, Literal, SizeUpdate, LiteralWithoutIndexing, Invalid };

struct HeaderField {
    static HeaderFieldType from_byte(uint8_t first_byte) {
        if (first_byte & 0x80) {
            return HeaderFieldType::Indexed;
        }
        if (first_byte & 0x40) {
            return HeaderFieldType::Literal;
        }
        if (first_byte & 0x20) {
            return HeaderFieldType::SizeUpdate;
        }
        if (first_byte <= 0x10) {
            return HeaderFieldType::LiteralWithoutIndexing;
        }
        return HeaderFieldType::Invalid;
    }
};

}  // namespace ion
