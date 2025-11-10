
#include <catch2/catch_test_macros.hpp>

#include "catch2/matchers/catch_matchers.hpp"
#include "headers/header_block_encoder.h"
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
