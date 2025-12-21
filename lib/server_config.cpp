#include "server_config.h"

namespace ion {

void ServerConfiguration::validate() const {
    if (!cleartext) {
        if (!cert_path) {
            throw std::runtime_error("TLS certificate path not set.");
        }
        if (!key_path) {
            throw std::runtime_error("TLS private key path not set.");
        }
    }
}

}  // namespace ion
