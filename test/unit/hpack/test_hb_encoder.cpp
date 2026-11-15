
#include <catch2/catch_test_macros.hpp>

#include "catch2/matchers/catch_matchers.hpp"
#include "hpack/header_block_encoder.h"
#include "http2_frames.h"

TEST_CASE("headers: encodes static table entries") {
    auto dynamic_table = ion::DynamicTable{};
    auto encoder = ion::HeaderBlockEncoder{dynamic_table};

    SECTION ("static table entries only (request headers)") {
        const auto hdrs = std::vector<ion::HttpHeader>{
            {":method", "GET"},
            {":scheme", "https"},
            {":path", "/"},
        };

        auto bytes = encoder.encode(hdrs);

        REQUIRE(bytes == std::vector<uint8_t>{0x82, 0x87, 0x84});
    }

    SECTION ("static table entries only (status)") {
        const auto hdrs = std::vector<ion::HttpHeader>{{":status", "200"}};

        auto bytes = encoder.encode(hdrs);

        REQUIRE(bytes == std::vector<uint8_t>{0x88});
    }
}

TEST_CASE("headers: encodes dynamic table entries") {
    auto dynamic_table = ion::DynamicTable{};
    auto decoder = ion::HeaderBlockEncoder{dynamic_table};

    SECTION ("returns dynamic header with static header name (not huffman)") {
        auto bytes = decoder.encode(std::vector<ion::HttpHeader>{
            {":authority", "foo"},
        });

        REQUIRE(bytes == std::vector<uint8_t>{0x41, 0x03, 0x66, 0x6f, 0x6f});
    }

    SECTION ("returns dynamic header with static header name (huffman)") {
        auto bytes = decoder.encode(std::vector<ion::HttpHeader>{
            {":authority", "localhost"},
        });

        REQUIRE(bytes == std::vector<uint8_t>{0x41, 0x86, 0xa0, 0xe4, 0x1d, 0x13, 0x9d, 0x9});
    }

    SECTION ("stores and returns dynamic header") {
        auto hdrs = std::vector<ion::HttpHeader>{
            {":authority", "foo"},
        };

        auto bytes = decoder.encode(hdrs);

        REQUIRE(bytes == std::vector<uint8_t>{0x41, 0x03, 0x66, 0x6f, 0x6f});

        auto bytes2 = decoder.encode(hdrs);

        REQUIRE(bytes2 == std::vector<uint8_t>{0xbe});
    }

    SECTION ("stores and returns dynamic header with indexed name when value changes") {
        auto hdrs1 = std::vector<ion::HttpHeader>{
            {"foo", "bar"},
        };

        auto bytes = decoder.encode(hdrs1);

        REQUIRE(bytes ==
                std::vector<uint8_t>{0x40, 0x03, 0x66, 0x6f, 0x6f, 0x03, 0x62, 0x61, 0x72});

        auto hdrs2 = std::vector<ion::HttpHeader>{
            {"foo", "baz"},
        };

        auto bytes2 = decoder.encode(hdrs2);

        REQUIRE(bytes2 == std::vector<uint8_t>{0x7e, 0x03, 0x62, 0x61, 0x7a});
    }
}

TEST_CASE("headers: encodes dynamic table indices beyond the integer prefix") {
    auto dynamic_table = ion::DynamicTable{};
    auto encoder = ion::HeaderBlockEncoder{dynamic_table};

    SECTION ("name index at the 6-bit boundary needs a continuation byte") {
        encoder.encode({{"a", "1"}});
        encoder.encode({{"b", "2"}});

        // "a" is now dynamic entry 1, so its table index is 61 + 1 + 1 = 63.
        const auto bytes = encoder.encode({{"a", "3"}});

        REQUIRE(bytes == std::vector<uint8_t>{0x7f, 0x00, 0x01, 0x33});
    }

    SECTION ("field index past the 7-bit prefix needs a continuation byte") {
        for (int i = 0; i < 66; i++) {
            encoder.encode({{"x" + std::to_string(i), "v"}});
        }

        // "x0" is now dynamic entry 65, so its table index is 61 + 65 + 1 = 127.
        const auto bytes = encoder.encode({{"x0", "v"}});

        REQUIRE(bytes == std::vector<uint8_t>{0xff, 0x00});
    }
}
