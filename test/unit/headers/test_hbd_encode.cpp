
#include <catch2/catch_test_macros.hpp>

#include "catch2/matchers/catch_matchers.hpp"
#include "headers/header_block_decoder.h"
#include "http2_frames.h"

TEST_CASE("headers: encodes static table entries") {
    auto dynamic_table = ion::DynamicTable{};
    auto encoder = ion::HeaderBlockDecoder{dynamic_table};

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
