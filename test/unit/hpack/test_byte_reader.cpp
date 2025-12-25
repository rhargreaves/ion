#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "hpack/byte_reader.h"

TEST_CASE("ByteReader") {
    std::vector<uint8_t> empty{};
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};

    SECTION ("read byte") {
        ByteReader br{data};
        REQUIRE(*br.read_byte() == 0x01);
        REQUIRE(*br.read_byte() == 0x02);
        REQUIRE(*br.read_byte() == 0x03);
    }

    SECTION ("read bytes") {
        ByteReader br{data};
        REQUIRE(std::ranges::equal(*br.read_bytes(2), std::vector<uint8_t>{0x01, 0x02}));
    }

    SECTION ("peek byte reads current byte without incrementing pointer") {
        ByteReader br{data};
        REQUIRE(*br.peek_byte() == 0x01);
        REQUIRE(*br.peek_byte() == 0x01);
    }

    SECTION ("has bytes returns false if empty") {
        ByteReader br{empty};
        REQUIRE_FALSE(br.has_bytes());
    }

    SECTION ("has bytes returns false if end of data") {
        ByteReader br{data};
        br.read_bytes(3);
        REQUIRE_FALSE(br.has_bytes());
    }

    SECTION ("has bytes returns true if data available") {
        ByteReader br{data};
        br.read_bytes(2);
        REQUIRE(br.has_bytes());
    }

    SECTION ("read bytes returns error if not enough bytes") {
        ByteReader br{data};
        auto res = br.read_bytes(4);

        REQUIRE(!res);
        REQUIRE(res.error() == ByteReaderError::NotEnoughBytes);
    }

    SECTION ("read byte returns error if not enough bytes") {
        ByteReader br{empty};
        auto res = br.read_byte();

        REQUIRE(!res);
        REQUIRE(res.error() == ByteReaderError::NotEnoughBytes);
    }

    SECTION ("peek byte returns error if not enough bytes") {
        ByteReader br{empty};
        auto res = br.peek_byte();

        REQUIRE(!res);
        REQUIRE(res.error() == ByteReaderError::NotEnoughBytes);
    }
};
