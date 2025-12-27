#include "byte_reader.h"

bool ByteReader::has_bytes(std::size_t count) const {
    return pos_ + count <= data_.size();
}

std::expected<uint8_t, ByteReaderError> ByteReader::read_byte() {
    if (!has_bytes()) {
        return std::unexpected(ByteReaderError::NotEnoughBytes);
    }
    return data_[pos_++];
}

std::expected<uint8_t, ByteReaderError> ByteReader::peek_byte() const {
    if (!has_bytes()) {
        return std::unexpected(ByteReaderError::NotEnoughBytes);
    }
    return data_[pos_];
}

std::expected<std::span<const uint8_t>, ByteReaderError> ByteReader::read_bytes(std::size_t count) {
    if (!has_bytes(count)) {
        return std::unexpected(ByteReaderError::NotEnoughBytes);
    }
    const auto result = data_.subspan(pos_, count);
    pos_ += count;

    // ReSharper disable once CppDFALocalValueEscapesFunction
    return result;
}
