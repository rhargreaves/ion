#pragma once
#include <cstdint>

namespace ion {

enum class HeaderFieldType {
    Indexed,
    LiteralIncremental,
    SizeUpdate,
    LiteralNeverIndex,
    LiteralNoIndex,
    Invalid
};

struct HeaderField {
    static HeaderFieldType from_byte(uint8_t first_byte) {
        if (first_byte & 0x80) {
            return HeaderFieldType::Indexed;
        }
        if (first_byte & 0x40) {
            return HeaderFieldType::LiteralIncremental;
        }
        if (first_byte & 0x20) {
            return HeaderFieldType::SizeUpdate;
        }
        if (first_byte & 0x10) {
            return HeaderFieldType::LiteralNeverIndex;
        }
        if (first_byte < 0x10) {
            return HeaderFieldType::LiteralNoIndex;
        }
        return HeaderFieldType::Invalid;
    }

    static std::string_view to_string(HeaderFieldType type) {
        switch (type) {
            case HeaderFieldType::Indexed:
                return "Indexed";
            case HeaderFieldType::LiteralIncremental:
                return "LiteralIncremental";
            case HeaderFieldType::SizeUpdate:
                return "SizeUpdate";
            case HeaderFieldType::LiteralNeverIndex:
                return "LiteralNeverIndex";
            case HeaderFieldType::LiteralNoIndex:
                return "LiteralNoIndex";
            default:
                return "Invalid";
        }
    }
};

}  // namespace ion
