#pragma once
#include <filesystem>
#include <optional>

namespace ion {

struct ServerConfiguration {
    std::optional<std::filesystem::path> cert_path;
    std::optional<std::filesystem::path> key_path;
    bool cleartext;

    void validate() const;
};

}  // namespace ion
