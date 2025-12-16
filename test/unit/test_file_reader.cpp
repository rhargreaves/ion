#include <unistd.h>

#include <catch2/catch_all.hpp>
#include <filesystem>

#include "file_reader.h"

using ion::FileReader;

namespace fs = std::filesystem;

TEST_CASE("sanitize_path blocks traversal and resolves safe paths") {
    auto tmp = fs::temp_directory_path();
    auto base = tmp / ("ion_sanitize_path_" + std::to_string(::getpid()));
    fs::create_directories(base / "sub");

    SECTION ("valid relative file inside base") {
        auto res = FileReader::sanitize_path(base.string(), "sub/file.txt");
        REQUIRE(res.has_value());
        auto canonical_base = fs::canonical(base);
        auto expected = fs::absolute(canonical_base / "sub/file.txt").string();
        CHECK(res.value() == expected);
    }

    SECTION ("directory traversal attempt using .. should be blocked") {
        auto res = FileReader::sanitize_path(base.string(), "sub/../..//secret.txt");
        CHECK_FALSE(res.has_value());
    }

    SECTION ("absolute requested path should be blocked") {
        auto abs_outside = fs::path("/") / "etc" / "passwd";  // unlikely to be under base
        auto res = FileReader::sanitize_path(base.string(), abs_outside.string());
        CHECK_FALSE(res.has_value());
    }

    SECTION ("non-existent base directory should return nullopt") {
        auto nonexistent = (base.parent_path() / "no_such_base_dir_for_ion_tests").string();
        auto res = FileReader::sanitize_path(nonexistent, "index.html");
        CHECK_FALSE(res.has_value());
    }

    fs::remove_all(base);
}
