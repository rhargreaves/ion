#pragma once
#include "file_store.h"

namespace ion {

class NativeFileStore : public FileStore {
   public:
    std::optional<std::vector<uint8_t>> read(const std::string& path) const override;
    std::optional<size_t> size(const std::string& path) const override;
    bool exists(const std::string& relative_path) const override;
};

}  // namespace ion
