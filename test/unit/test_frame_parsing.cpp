#include <catch2/catch_test_macros.hpp>

#include "http2_frames.h"

TEST_CASE("HTTP/2 frame header parsing", "[frames]") {
    SECTION ("can parse frame header") {
        std::array<uint8_t, 9> data = {
            0x00, 0x00, 0x0C,       // Length: 12
            0x04,                   // Type: SETTINGS
            0x00,                   // Flags: none
            0x00, 0x00, 0x00, 0x00  // Stream ID: 0
        };

        auto header = ion::Http2FrameHeader::parse(data);

        REQUIRE(header.length == 12);
        REQUIRE(header.type == 4);
        REQUIRE(header.flags == 0);
        REQUIRE(header.stream_id == 0);
    }
}
