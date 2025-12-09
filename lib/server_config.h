#pragma once
#include <filesystem>

namespace ion {

struct ServerConfiguration {
    std::filesystem::path cert_path;
    std::filesystem::path key_path;

    static ServerConfiguration from_env();
};

}  // namespace ion
