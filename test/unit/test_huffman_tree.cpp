#include <array>
#include <catch2/catch_test_macros.hpp>

#include "bit_reader.h"
#include "huffman_tree.h"

TEST_CASE("huffman_tree: builds tree and decodes correctly") {
    HuffmanTree tree{};

    tree.insert_symbol(0, 0b1111'1111'1100'0000'0000'0000'0000'0000, 13);
    tree.insert_symbol(35, 0b1111'1111'1010'0000'0000'0000'0000'0000, 12);

    std::array<uint8_t, 4> encoded = {0b11111111, 0b11000'111, 0b11111101, 0x0};

    auto symbols = tree.decode(encoded);

    REQUIRE(symbols.size() == 2);
}

TEST_CASE("bit_reader: reads bits correctly") {
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
