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
    return (1 << bits) - 1;
}

std::expected<uint32_t, IntegerDecodeError> IntegerDecoder::decode(ByteReader& reader,
                                                                   uint8_t prefix_bits) {
    uint32_t sum{};
    const auto mask = make_mask(prefix_bits);

    const auto byte_res = reader.read_byte();
    if (!byte_res) {
        return std::unexpected(IntegerDecodeError::NotEnoughBytes);
    }

    const uint8_t prefix = *byte_res & mask;
    sum += prefix;
    bool cont = prefix == mask;

    uint32_t shift = 0;
    while (cont) {
        auto continuation_res = reader.read_byte();
        if (!continuation_res) {
            return std::unexpected(IntegerDecodeError::NotEnoughBytes);
        }

        const uint32_t value = *continuation_res & 0x7F;

        if (value > (std::numeric_limits<uint32_t>::max() - sum) >> shift) {
            return std::unexpected(IntegerDecodeError::IntegerOverflow);
        }

        sum += value << shift;
        shift += 7;
        cont = *continuation_res & 0x80;
    }
    return sum;
}

}  // namespace ion
