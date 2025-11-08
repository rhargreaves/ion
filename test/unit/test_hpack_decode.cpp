#include <catch2/catch_test_macros.hpp>

#include "header_block_decoder.h"
#include "http2_frames.h"

void check_header(std::vector<HttpHeader>& hdrs, size_t index, const std::string& expectedName,
                  const std::string& expectedValue) {
    REQUIRE(hdrs[index].name == expectedName);
    REQUIRE(hdrs[index].value == expectedValue);
};

TEST_CASE("headers: decodes static table entries") {
    std::array<uint8_t, 4> data = {0x82, 0x84, 0xbe, 0x87};

    auto decoder = HeaderBlockDecoder{};
    auto hdrs = decoder.decode(data);

    REQUIRE(hdrs.size() == 3);  // skip over non-static table entries

    SECTION ("parses :method: GET") {
        check_header(hdrs, 0, ":method", "GET");
    }

    SECTION ("parses :path: /") {
        check_header(hdrs, 1, ":path", "/");
    }

    SECTION ("parses :scheme: https") {
        check_header(hdrs, 2, ":scheme", "https");
    }
}

TEST_CASE("headers: decodes static table entries (boundaries)") {
    std::array<uint8_t, 2> data = {0x81, 0xbd};

    auto decoder = HeaderBlockDecoder{};
    auto hdrs = decoder.decode(data);

    REQUIRE(hdrs.size() == 2);

    SECTION ("parses :authority: <empty>") {
        check_header(hdrs, 0, ":authority", "");
    }

    SECTION ("parses www-authenticate: <empty>") {
        check_header(hdrs, 1, "www-authenticate", "");
    }
}

TEST_CASE("headers: ignores out-of-bounds static table entries") {
    std::array<uint8_t, 2> data = {0x80, 0xbe};

    auto decoder = HeaderBlockDecoder{};
    REQUIRE(decoder.decode(data).empty());
}
