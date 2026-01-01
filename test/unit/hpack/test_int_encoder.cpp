#include <algorithm>
#include <catch2/catch_test_macros.hpp>

#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_vector.hpp"
#include "hpack/int_encoder.h"

TEST_CASE("integer encoder: encoders ints within prefix range") {
    SECTION ("prefix 5") {
        auto bytes = ion::IntegerEncoder::encode(30, 5);

        REQUIRE_THAT(bytes, Catch::Matchers::Equals(std::vector<uint8_t>{0x1e}));
    }

    SECTION ("prefix 7") {
        auto bytes = ion::IntegerEncoder::encode(126, 7);

        REQUIRE_THAT(bytes, Catch::Matchers::Equals(std::vector<uint8_t>{0x7e}));
    }
}

TEST_CASE("integer encoder: encoders ints outside of prefix range") {
    SECTION ("prefix 5; edge") {
        auto bytes = ion::IntegerEncoder::encode(31, 5);

        REQUIRE_THAT(bytes, Catch::Matchers::Equals(std::vector<uint8_t>{0x1f, 0x00}));
    }

    SECTION ("prefix 7; edge") {
        auto bytes = ion::IntegerEncoder::encode(127, 7);

        REQUIRE_THAT(bytes, Catch::Matchers::Equals(std::vector<uint8_t>{0x7f, 0x00}));
    }

    SECTION ("prefix 7") {
        auto bytes = ion::IntegerEncoder::encode(1023, 7);

        REQUIRE_THAT(bytes, Catch::Matchers::Equals(std::vector<uint8_t>{0x7f, 0x80, 0x07}));
    }
}

TEST_CASE("integer encoder: throws exceptions when prefix out of range") {
    SECTION ("prefix 0") {
        REQUIRE_THROWS_AS(ion::IntegerEncoder::encode(30, 0), std::out_of_range);
    }

    SECTION ("prefix 9") {
        REQUIRE_THROWS_AS(ion::IntegerEncoder::encode(126, 9), std::out_of_range);
    }
}
