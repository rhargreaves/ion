#include <catch2/catch_test_macros.hpp>

#include "bit_reader.h"

TEST_CASE("bit reader reads bits correctly") {
    constexpr std::array<const uint8_t, 2> encoded = {0x9f, 0xfa};
    BitReader br{encoded};

    constexpr std::array expected_bits = {
        true, false, false, true, true, true,  true, true,   // 0x9f
        true, true,  true,  true, true, false, true, false,  // 0xfa
    };

    for (const auto bit : expected_bits) {
        REQUIRE(br.has_more());
        REQUIRE(br.read_bit() == bit);
    }
}
