#include "test_helpers.h"

#include <filesystem>

#include "spdlog/spdlog.h"

ion::Http2Server TestHelpers::create_test_server() {
    spdlog::set_level(spdlog::level::err);

    return ion::Http2Server{ion::ServerConfiguration{
        .cert_path = std::filesystem::path{std::getenv("ION_TLS_CERT_PATH")},
        .key_path = std::filesystem::path{std::getenv("ION_TLS_KEY_PATH")},
    }};
}
