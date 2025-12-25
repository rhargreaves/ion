#pragma once

#include <span>

class ByteReader {
   public:
    explicit ByteReader(std::span<const uint8_t> data) : data_(data), pos_(0) {}

    ByteReader(const ByteReader&) = delete;
    ByteReader& operator=(const ByteReader&) = delete;

    ByteReader(ByteReader&&) = default;
    ByteReader& operator=(ByteReader&&) = default;

    [[nodiscard]] bool has_bytes(size_t count = 1) const;
    uint8_t read_byte();
    [[nodiscard]] uint8_t peek_byte() const;
    std::span<const uint8_t> read_bytes(size_t count);

   private:
    std::span<const uint8_t> data_;
    size_t pos_;
};
