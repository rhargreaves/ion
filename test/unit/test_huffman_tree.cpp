#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include "bit_reader.h"
#include "huffman_tree.h"

TEST_CASE("huffman_tree: builds tree and decodes correctly") {
    HuffmanTree tree{};

    tree.insert_symbol(0, 0x1ff8, 13);
    tree.insert_symbol(35, 0xffa, 12);

    std::array<uint8_t, 4> encoded = {0b11111111, 0b11000'111, 0b11111101, 0x0};

    auto symbols = tree.decode(encoded, 25);

    REQUIRE(symbols.size() == 2);
}

TEST_CASE("huffman_tree: throws on invalid code") {
    HuffmanTree tree{};

    tree.insert_symbol(0, 0x1, 1);

    std::array<uint8_t, 1> encoded = {0x0};

    CHECK_THROWS_MATCHES(tree.decode(encoded, 1), std::runtime_error,
                         Catch::Matchers::Message("Invalid Huffman code"));
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
