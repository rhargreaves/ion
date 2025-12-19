#include <catch2/catch_test_macros.hpp>

#include "hpack/byte_reader.h"
#include "hpack/int_decoder.h"

TEST_CASE("integer decoder: decodes ints with prefix range") {
    SECTION ("prefix 8") {
        constexpr auto bytes = std::to_array<uint8_t>({0x7f});

        ByteReader br{bytes};

        REQUIRE(*ion::IntegerDecoder::decode(br, 8) == 127);
    }

    SECTION ("prefix 6") {
        constexpr auto bytes = std::to_array<uint8_t>({0x7f, 0x06});

        ByteReader br{bytes};

        REQUIRE(*ion::IntegerDecoder::decode(br, 6) == 69);
    }

    SECTION ("prefix 6 (LSB bit 0)") {
        constexpr auto bytes = std::to_array<uint8_t>({0x7e});

        ByteReader br{bytes};

        REQUIRE(*ion::IntegerDecoder::decode(br, 6) == 62);
    }

    SECTION ("prefix 4") {
        constexpr auto bytes = std::to_array<uint8_t>({0xf2});

        ByteReader br{bytes};

        REQUIRE(*ion::IntegerDecoder::decode(br, 4) == 2);
    }

    SECTION ("prefix 2") {
        constexpr auto bytes = std::to_array<uint8_t>({0xfd});

        ByteReader br{bytes};

        REQUIRE(*ion::IntegerDecoder::decode(br, 2) == 1);
    }

    SECTION ("prefix 0 (not expected)") {
        constexpr auto bytes = std::to_array<uint8_t>({0x12});

        ByteReader br{bytes};

        REQUIRE(*ion::IntegerDecoder::decode(br, 0) == 0);
    }
}

TEST_CASE("integer decoder: decodes ints outside prefix range") {
    SECTION ("prefix 8") {
        constexpr auto bytes = std::to_array<uint8_t>({0xff, 0x01});

        ByteReader br{bytes};

        REQUIRE(*ion::IntegerDecoder::decode(br, 8) == 256);
    }

    SECTION ("prefix 4") {
        constexpr auto bytes = std::to_array<uint8_t>({0xff, 0xff, 0x01});

        ByteReader br{bytes};

        REQUIRE(*ion::IntegerDecoder::decode(br, 4) == 15 + 127 + 128);
    }

    SECTION ("not enough bytes") {
        constexpr auto bytes = std::to_array<uint8_t>({0xff});

        ByteReader br{bytes};

        REQUIRE(ion::IntegerDecoder::decode(br, 8).error() ==
                ion::IntegerDecodeError::NotEnoughBytes);
    }
}
