
#include <catch2/catch_test_macros.hpp>

#include "hpack/dynamic_table.h"

TEST_CASE("inserts new headers at the front") {
    ion::DynamicTable table{};
    table.insert(ion::HttpHeader{"foo", "bar"});

    const auto most_recent_hdr = ion::HttpHeader{"foo", "baz"};
    table.insert(most_recent_hdr);

    REQUIRE(table.find(most_recent_hdr).value() == 0);
}
