#pragma once
#include <filesystem>
#include <optional>

#include "router.h"

namespace ion {

struct ServerConfiguration {
    std::optional<std::filesystem::path> cert_path;
    std::optional<std::filesystem::path> key_path;
    bool cleartext;
    std::string custom_404_path;

    void validate() const;
};

}  // namespace ion
