#include "int_decoder.h"

#include "spdlog/spdlog.h"

namespace ion {

constexpr uint8_t make_mask(uint8_t bits) {
    if (bits == 0) {
        return 0;
    }
    if (bits >= 8) {
        return 0xff;
    }
    return static_cast<uint8_t>((1 << bits) - 1);
}

std::expected<uint32_t, IntegerDecodeError> IntegerDecoder::decode(ByteReader& reader,
                                                                   uint8_t prefix_bits) {
    uint32_t sum{};
    const auto mask = make_mask(prefix_bits);

    const uint8_t prefix = reader.read_byte() & mask;
    bool continues = prefix == mask;
    sum += prefix;

    while (continues) {
        if (reader.has_bytes() == 0) {
            return std::unexpected(IntegerDecodeError::NotEnoughBytes);
        }
        const uint8_t continuation = reader.read_byte();
        continues = continuation == make_mask(8);
        sum += continuation;
    }

    return sum;
}

}  // namespace ion
