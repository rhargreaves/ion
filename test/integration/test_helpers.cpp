#include "test_helpers.h"

#include <filesystem>

ion::Http2Server TestHelpers::create_test_server() {
    return ion::Http2Server{ion::ServerConfiguration{
        .cert_path = std::filesystem::path{std::getenv("ION_TLS_CERT_PATH")},
        .key_path = std::filesystem::path{std::getenv("ION_TLS_KEY_PATH")},
    }};
}
