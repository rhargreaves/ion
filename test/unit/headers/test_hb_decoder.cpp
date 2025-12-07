#include <catch2/catch_test_macros.hpp>

#include "headers/header_block_decoder.h"
#include "http2_frames.h"

void check_header(std::vector<ion::HttpHeader>& hdrs, size_t index, const std::string& expectedName,
                  const std::string& expectedValue) {
    REQUIRE(hdrs[index].name == expectedName);
    REQUIRE(hdrs[index].value == expectedValue);
};

TEST_CASE("headers: decodes static table entries") {
    auto dynamic_table = ion::DynamicTable{};
    auto decoder = ion::HeaderBlockDecoder{dynamic_table};

    SECTION ("static table entries only") {
        std::array<uint8_t, 4> data = {0x82, 0x84, 0xbe, 0x87};
        auto hdrs = decoder.decode(data);

        REQUIRE(hdrs.size() == 3);  // skip over any dynamic table entries not added yet

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

    SECTION ("literal header field with indexed name") {
        std::array<uint8_t, 8> data = {0x41, 0x86, 0xa0, 0xe4, 0x1d, 0x13, 0x9d, 0x9};

        auto hdrs = decoder.decode(data);

        REQUIRE(hdrs.size() == 1);

        SECTION ("parses :authority: localhost") {
            check_header(hdrs, 0, ":authority", "localhost");
        }
    }

    SECTION ("literal header field without indexing") {
        auto data = std::to_array<uint8_t>({0x04, 0x84, 0x62, 0x33, 0xc9, 0xeb});

        auto hdrs = decoder.decode(data);

        REQUIRE(hdrs.size() == 1);

        SECTION ("parses :path: /body") {
            check_header(hdrs, 0, ":path", "/body");
        }
    }

    SECTION ("handles min/max indices") {
        std::array<uint8_t, 2> data = {0x81, 0xbd};
        auto hdrs = decoder.decode(data);

        REQUIRE(hdrs.size() == 2);

        SECTION ("parses :authority: <empty>") {
            check_header(hdrs, 0, ":authority", "");
        }

        SECTION ("parses www-authenticate: <empty>") {
            check_header(hdrs, 1, "www-authenticate", "");
        }
    }

    SECTION ("handles out-of-range indices") {
        std::array<uint8_t, 2> data = {0x80, 0xbe};

        REQUIRE(decoder.decode(data).empty());
    }
}

TEST_CASE("headers: decodes dynamic table entries") {
    auto dynamic_table = ion::DynamicTable{};
    auto decoder = ion::HeaderBlockDecoder{dynamic_table};

    SECTION ("stores and returns dynamic header with static header name") {
        constexpr auto req1 = std::to_array<uint8_t>({
            /* :method: GET */ 0x82,                                                    //
            /* :path: / */ 0x84,                                                        //
            /* :authority: localhost */ 0x41, 0x86, 0xa0, 0xe4, 0x1d, 0x13, 0x9d, 0x9,  //
            /* :scheme: https */ 0x87                                                   //
        });
        auto hdrs1 = decoder.decode(req1);

        REQUIRE(hdrs1.size() == 4);
        check_header(hdrs1, 0, ":method", "GET");
        check_header(hdrs1, 1, ":path", "/");
        check_header(hdrs1, 2, ":authority", "localhost");
        check_header(hdrs1, 3, ":scheme", "https");

        constexpr auto req2 = std::to_array<uint8_t>({0x82, 0x84, 0xbe, 0x87});
        auto hdrs2 = decoder.decode(req2);

        REQUIRE(hdrs1.size() == hdrs2.size());
        for (size_t i = 0; i < hdrs1.size(); ++i) {
            REQUIRE(hdrs1[i].name == hdrs2[i].name);
            REQUIRE(hdrs1[i].value == hdrs2[i].value);
        }
    }

    SECTION ("literal header field - dynamic header name") {
        constexpr auto req1 =
            std::to_array<uint8_t>({0x40, 0x82, 0x94, 0xe7, 0x83, 0x8c, 0x76, 0x7f});
        auto hdrs1 = decoder.decode(req1);

        REQUIRE(hdrs1.size() == 1);
        check_header(hdrs1, 0, "foo", "bar");

        constexpr auto req2 =
            std::to_array<uint8_t>({0x7e, 0x91, 0x8c, 0x76, 0x29, 0x18, 0xfd, 0xa9, 0x18, 0xec,
                                    0x52, 0x31, 0xfb, 0x52, 0x31, 0xd8, 0xa4, 0x63, 0xf7});
        auto hdrs2 = decoder.decode(req2);

        REQUIRE(hdrs2.size() == 1);
        check_header(hdrs2, 0, "foo", "bar baz bar baz bar baz");
    }

    SECTION ("literal header field - new name (short value, non-huffman)") {
        constexpr auto foo_hdr =
            std::to_array<uint8_t>({0x40, 0x84, 0xf2, 0xb4, 0xa7, 0x3f, 0x3, 0x62, 0x61, 0x72});

        auto hdrs = decoder.decode(foo_hdr);

        REQUIRE(hdrs.size() == 1);
        check_header(hdrs, 0, "x-foo", "bar");
    }

    SECTION ("literal header field - new name (long value, huffman)") {
        constexpr auto foo_hdr = std::to_array<uint8_t>(
            {0x40, 0x84, 0xf2, 0xb4, 0xa7, 0x3f, 0x9a, 0x94, 0xe7, 0x52, 0x31,
             0xd8, 0xa4, 0x63, 0xf6, 0xa4, 0xa7, 0x3a, 0x91, 0x8e, 0xc5, 0x23,
             0x1f, 0xb5, 0x25, 0x39, 0xd4, 0x8c, 0x76, 0x29, 0x18, 0xfd, 0xff});

        auto hdrs = decoder.decode(foo_hdr);

        REQUIRE(hdrs.size() == 1);
        check_header(hdrs, 0, "x-foo", "foo bar baz foo bar baz foo bar baz");
    }

    SECTION ("literal header field - new name, without indexing (long value, huffman)") {
        constexpr auto foo_hdr = std::to_array<uint8_t>(
            {0x00, 0x84, 0xf2, 0xb4, 0xa7, 0x3f, 0x9a, 0x94, 0xe7, 0x52, 0x31,
             0xd8, 0xa4, 0x63, 0xf6, 0xa4, 0xa7, 0x3a, 0x91, 0x8e, 0xc5, 0x23,
             0x1f, 0xb5, 0x25, 0x39, 0xd4, 0x8c, 0x76, 0x29, 0x18, 0xfd, 0xff});

        auto hdrs = decoder.decode(foo_hdr);

        REQUIRE(hdrs.size() == 1);
        check_header(hdrs, 0, "x-foo", "foo bar baz foo bar baz foo bar baz");
    }
}
