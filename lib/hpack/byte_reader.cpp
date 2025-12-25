#include "byte_reader.h"

bool ByteReader::has_bytes(size_t count) const {
    return pos_ + count <= data_.size();
}

uint8_t ByteReader::read_byte() {
    if (!has_bytes()) {
        throw std::out_of_range("Attempted to read beyond buffer");
    }
    return data_[pos_++];
}

uint8_t ByteReader::peek_byte() const {
    if (!has_bytes()) {
        throw std::out_of_range("Attempted to read beyond buffer");
    }
    return data_[pos_];
}

std::span<const uint8_t> ByteReader::read_bytes(size_t count) {
    if (!has_bytes(count)) {
        throw std::out_of_range("Attempted to read beyond buffer");
    }
    const auto result = data_.subspan(pos_, count);
    pos_ += count;

    // ReSharper disable once CppDFALocalValueEscapesFunction
    return result;
}
