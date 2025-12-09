#include "server_config.h"

namespace ion {

ServerConfiguration ServerConfiguration::from_env() {
    const char* cert_path = std::getenv("ION_TLS_CERT_PATH");
    const char* key_path = std::getenv("ION_TLS_KEY_PATH");

    if (!cert_path || !key_path) {
        throw std::runtime_error(
            "Environment variables ION_TLS_CERT_PATH and ION_TLS_KEY_PATH must be set");
    }

    return ServerConfiguration{.cert_path = std::filesystem::path(cert_path),
                               .key_path = std::filesystem::path(key_path)};
}

}  // namespace ion
