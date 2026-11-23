#pragma once
#include <optional>
#include <string_view>
#include <vector>

namespace ion {

class FileStore {
   public:
    virtual ~FileStore() = default;
    virtual std::optional<std::vector<uint8_t>> read(const std::string& path) const = 0;
    virtual std::optional<size_t> size(const std::string& path) const = 0;
    virtual bool exists(const std::string& relative_path) const = 0;
};

}  // namespace ion
