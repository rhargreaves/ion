
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

    SECTION ("stores and returns dynamic header with static header name (not huffman)") {
        auto bytes = decoder.encode(std::vector<ion::HttpHeader>{
            {":authority", "foo"},
        });

        REQUIRE(bytes == std::vector<uint8_t>{0x41, 0x03, 0x66, 0x6f, 0x6f});
    }

    SECTION ("stores and returns dynamic header with static header name (huffman)") {
        auto bytes = decoder.encode(std::vector<ion::HttpHeader>{
            {":authority", "localhost"},
        });

        REQUIRE(bytes == std::vector<uint8_t>{0x41, 0x86, 0xa0, 0xe4, 0x1d, 0x13, 0x9d, 0x9});
    }
}
