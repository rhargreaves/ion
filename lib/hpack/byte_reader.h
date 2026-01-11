#pragma once

#include <cstdint>
#include <expected>
#include <span>

enum class ByteReaderError { NotEnoughBytes };

class ByteReader {
   public:
    explicit ByteReader(std::span<const uint8_t> data) : data_(data), pos_(0) {}

    ByteReader(const ByteReader&) = delete;
    ByteReader& operator=(const ByteReader&) = delete;

    ByteReader(ByteReader&&) = default;
    ByteReader& operator=(ByteReader&&) = default;

    [[nodiscard]] bool has_bytes(std::size_t count = 1) const;
    std::expected<uint8_t, ByteReaderError> read_byte();
    [[nodiscard]] std::expected<uint8_t, ByteReaderError> peek_byte() const;
    std::expected<std::span<const uint8_t>, ByteReaderError> read_bytes(std::size_t count);

   private:
    std::span<const uint8_t> data_;
    std::size_t pos_;
};
