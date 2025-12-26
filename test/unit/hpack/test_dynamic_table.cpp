
#include <catch2/catch_test_macros.hpp>

#include "hpack/dynamic_table.h"

TEST_CASE("DynamicTable") {
    ion::DynamicTable table{};

    SECTION ("inserts new header at the front") {
        table.insert(ion::HttpHeader{"foo", "bar"});

        const auto most_recent_hdr = ion::HttpHeader{"foo", "baz"};
        table.insert(most_recent_hdr);

        REQUIRE(table.find(most_recent_hdr).value() == 0);
    }

    SECTION ("returns count of entries") {
        table.insert(ion::HttpHeader{"foo", "bar"});

        REQUIRE(table.count() == 1);
    }

    SECTION ("returns table size (in octets)") {
        table.insert(ion::HttpHeader{"foo", "bar"});

        constexpr auto expected_size = 3 + 3 + 32;

        REQUIRE(table.size() == expected_size);
    }

    SECTION ("evicts old headers when table size exceeds max") {
        table = ion::DynamicTable{50};

        table.insert(ion::HttpHeader{"foo", "bar"});  // 38
        table.insert(ion::HttpHeader{"fo1", "b"});    // 36

        REQUIRE(table.count() == 1);
        REQUIRE(table.size() == 36);
    }

    SECTION ("wipes whole table when inserted entry is bigger than max size") {
        table = ion::DynamicTable{50};

        table.insert(ion::HttpHeader{"foo", "baaar"});  // 40 (3 + 5 + 32)
        table.insert(
            ion::HttpHeader{std::string(8, 'A'), std::string(11, 'A')});  // 52 (8 + 11 + 32)

        REQUIRE(table.count() == 0);
        REQUIRE(table.size() == 0);
    }

    SECTION ("table is empty and entry NOT added if larger than max_table_size_") {
        table = ion::DynamicTable{100};

        table.insert(ion::HttpHeader{"small", "header"});  // 5+6+32 = 43 octets
        REQUIRE(table.count() == 1);
        REQUIRE(table.size() == 43);

        // Name (30) + Value (39) + Overhead (32) = 101
        ion::HttpHeader huge_header{std::string(30, 'a'), std::string(39, 'b')};
        table.insert(huge_header);

        CHECK(table.count() == 0);
        CHECK(table.size() == 0);

        auto result = table.find(huge_header);
        CHECK_FALSE(result.has_value());
    }

    SECTION ("table resized with headers evicted to maintain new max size") {
        table = ion::DynamicTable{120};

        table.insert(ion::HttpHeader{"1234", "1234"});  // 4 + 4 + 32 = 40
        table.insert(ion::HttpHeader{"a234", "1234"});  // 4 + 4 + 32 = 40
        table.insert(ion::HttpHeader{"b234", "1234"});  // 4 + 4 + 32 = 40

        table.set_max_table_size(81);
        REQUIRE(table.size() == 80);
        REQUIRE(table.count() == 2);

        table.set_max_table_size(41);
        REQUIRE(table.size() == 40);
        REQUIRE(table.count() == 1);

        table.set_max_table_size(0);
        REQUIRE(table.size() == 0);
        REQUIRE(table.count() == 0);
    }
}
