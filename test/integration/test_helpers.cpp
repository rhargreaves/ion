#include "test_helpers.h"

#include <filesystem>

#include "spdlog/spdlog.h"

ion::Http2Server TestHelpers::create_test_server() {
    spdlog::set_level(spdlog::level::err);

    const char* cert_env = std::getenv("ION_TLS_CERT_PATH");
    const char* key_env = std::getenv("ION_TLS_KEY_PATH");

    const auto config = ion::ServerConfiguration{
        .cert_path = cert_env ? std::optional{std::filesystem::path{cert_env}} : std::nullopt,
        .key_path = key_env ? std::optional{std::filesystem::path{key_env}} : std::nullopt,
    };

    config.validate();

    return ion::Http2Server{config};
}
