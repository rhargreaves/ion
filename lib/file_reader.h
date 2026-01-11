#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ion {

class FileReader {
   public:
    static std::optional<std::vector<uint8_t>> read_file(const std::string& path);
    static std::optional<size_t> get_file_size(const std::string& path);
    static std::string get_mime_type(const std::string& path);
    static bool is_readable(const std::string& path);
    static std::optional<std::string> sanitize_path(const std::string& base_dir,
                                                    const std::string& requested_path);
};

}  // namespace ion
