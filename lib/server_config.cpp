#include "server_config.h"

namespace ion {

ServerConfiguration ServerConfiguration::from_env() {
    const char* cert_path = std::getenv("ION_TLS_CERT_PATH");
    const char* key_path = std::getenv("ION_TLS_KEY_PATH");

    return ServerConfiguration{
        .cert_path = cert_path ? std::optional{std::filesystem::path(cert_path)} : std::nullopt,
        .key_path = key_path ? std::optional{std::filesystem::path(key_path)} : std::nullopt};
}

void ServerConfiguration::validate() const {
    if (!cleartext) {
        if (!cert_path) {
            throw std::runtime_error(
                "TLS certificate path not defined. Set ION_TLS_CERT_PATH env var.");
        }
        if (!key_path) {
            throw std::runtime_error(
                "TLS private key path not defined. Set ION_TLS_KEY_PATH env var.");
        }
    }
}

}  // namespace ion
