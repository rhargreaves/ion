#include <catch2/catch_test_macros.hpp>

#include "header_block_decoder.h"
#include "http2_frames.h"

TEST_CASE("headers: decodes static table entries") {
    std::array<uint8_t, 9> data = {0x82, 0x84, 0xbe, 0x87};

    auto decoder = HeaderBlockDecoder{};
    auto hdrs = decoder.decode(data);

    REQUIRE(hdrs.size() == 3);  // skip over non-static table entries

    SECTION("parses :method: GET") {
        REQUIRE(hdrs[0].name == ":method");
        REQUIRE(hdrs[0].value == "GET");
    }

    SECTION("parses :path: /") {
        REQUIRE(hdrs[1].name == ":path");
        REQUIRE(hdrs[1].value == "/");
    }

    SECTION("parses :scheme: https") {
        REQUIRE(hdrs[2].name == ":scheme");
        REQUIRE(hdrs[2].value == "https");
    }
}
