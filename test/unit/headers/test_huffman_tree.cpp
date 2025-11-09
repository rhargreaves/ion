#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <iostream>

#include "headers/bit_writer.h"
#include "headers/huffman_codes.h"
#include "headers/huffman_tree.h"

TEST_CASE("huffman tree builds & decodes correctly") {
    ion::HuffmanTree tree{};

    SECTION ("basic inserts & decodes (2 symbols)") {
        tree.insert_symbol(0, 0b1111'1111'1100'0, 13);
        tree.insert_symbol(35, 0b1111'1111'1010, 12);

        std::array<uint8_t, 4> encoded = {0b1111'1111, 0b1100'0111, 0b1111'1101, 0b0111'1111};

        auto symbols = tree.decode(encoded);

        REQUIRE(symbols.size() == 2);
        REQUIRE(symbols[0] == 0);
        REQUIRE(symbols[1] == 35);
    }

    SECTION ("handles symbol with all zeros (space symbol)") {
        tree.insert_symbol(48, 0x00, 5);

        ion::BitWriter writer;
        writer.write_bits(0x00, 5);
        auto test_data = writer.finalize();

        auto symbols = tree.decode(test_data);

        REQUIRE(symbols.size() == 1);
        REQUIRE(symbols[0] == 48);
    }

    SECTION ("handles all HPACK-defined codes") {
        ion::BitWriter writer;
        for (size_t i = 0; i < ion::HUFFMAN_CODES.size(); i++) {
            const auto& code = ion::HUFFMAN_CODES[i];
            tree.insert_symbol(static_cast<int16_t>(i), code.lsb_aligned_code, code.code_len);
            writer.write_bits(code.lsb_aligned_code, code.code_len);
        }
        auto bitstream = writer.finalize();
        auto symbols = tree.decode(bitstream);

        REQUIRE(symbols.size() == ion::HUFFMAN_CODES.size());
    }

    SECTION ("throws on invalid code that isn't padding") {
        tree.insert_symbol(0, 0x1, 1);

        std::array<uint8_t, 1> encoded = {0x0};

        CHECK_THROWS_MATCHES(tree.decode(encoded), std::runtime_error,
                             Catch::Matchers::Message(
                                 "remaining unmatched Huffman code is not padding: (bit pos = 1)"));
    }
}
