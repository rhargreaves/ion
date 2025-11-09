#pragma once

#include <span>

class ByteReader {
   public:
    explicit ByteReader(std::span<const uint8_t> data) : data_(data), pos_(0) {}

    bool has_bytes(size_t count = 1) const {
        return pos_ + count <= data_.size();
    }
    uint8_t read_byte() {
        if (!has_bytes()) {
            throw std::out_of_range("Attempted to read beyond buffer");
        }
        return data_[pos_++];
    }

    std::span<const uint8_t> read_bytes(size_t count) {
        if (!has_bytes(count)) {
            throw std::out_of_range("Attempted to read beyond buffer");
        }
        auto result = data_.subspan(pos_, count);
        pos_ += count;
        return result;
    }

    size_t position() const {
        return pos_;
    }
    size_t remaining() const {
        return data_.size() - pos_;
    }

   private:
    std::span<const uint8_t> data_;
    size_t pos_;
};
