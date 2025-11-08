#include <catch2/catch_test_macros.hpp>

#include "header_block_decoder.h"
#include "http2_frames.h"

TEST_CASE("headers: decodes static table entries") {
    std::array<uint8_t, 9> data = {0x82, 0x84, 0xbe, 0x87};

    auto decoder = HeaderBlockDecoder{};
    auto hdrs = decoder.decode(data);

    auto checkHeader = [&](size_t index, const std::string& expectedName,
                           const std::string& expectedValue) {
        REQUIRE(hdrs[index].name == expectedName);
        REQUIRE(hdrs[index].value == expectedValue);
    };

    REQUIRE(hdrs.size() == 3);  // skip over non-static table entries

    SECTION ("parses :method: GET") {
        checkHeader(0, ":method", "GET");
    }

    SECTION ("parses :path: /") {
        checkHeader(1, ":path", "/");
    }

    SECTION ("parses :scheme: https") {
        checkHeader(2, ":scheme", "https");
    }
}
