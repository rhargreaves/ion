#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <iostream>

#include "bit_reader.h"
#include "bit_writer.h"
#include "huffman_codes.h"
#include "huffman_tree.h"

TEST_CASE("huffman_tree: builds tree and decodes correctly") {
    HuffmanTree tree{};

    tree.insert_symbol(0, 0b1111'1111'1100'0, 13);
    tree.insert_symbol(35, 0b1111'1111'1010, 12);

    std::array<uint8_t, 4> encoded = {0b1111'1111, 0b1100'0111, 0b1111'1101, 0b0111'1111};

    auto symbols = tree.decode(encoded);

    REQUIRE(symbols.size() == 2);
}

TEST_CASE("huffman_tree: builds tree and decodes correctly (space)") {
    HuffmanTree tree{};

    tree.insert_symbol(48, 0x00, 5);

    BitWriter writer;
    writer.write_bits(0x00, 5);
    auto test_data = writer.finalize();

    auto symbols = tree.decode(test_data);

    REQUIRE(symbols.size() == 1);
    REQUIRE(symbols[0] == 48);
}

TEST_CASE("huffman_tree: builds tree and decodes correctly (all possible symbols)") {
    HuffmanTree tree{};
    BitWriter writer;
    for (size_t i = 0; i < HUFFMAN_CODES.size(); i++) {
        const auto& code = HUFFMAN_CODES[i];
        // build tree
        tree.insert_symbol(static_cast<int16_t>(i), code.lsb_aligned_code, code.code_len);

        // build one line bitstream
        writer.write_bits(code.lsb_aligned_code, code.code_len);
    }
    auto bitstream = writer.finalize();
    auto symbols = tree.decode(bitstream);

    REQUIRE(symbols.size() == HUFFMAN_CODES.size());
}

TEST_CASE("huffman_tree: throws on invalid code") {
    HuffmanTree tree{};

    tree.insert_symbol(0, 0x1, 1);

    std::array<uint8_t, 1> encoded = {0x0};

    CHECK_THROWS_MATCHES(
        tree.decode(encoded), std::runtime_error,
        Catch::Matchers::Message("remaining unmatched Huffman code is not padding: (bit pos = 1)"));
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
