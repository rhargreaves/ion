#include "file_reader.h"

#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace ion {

std::optional<std::vector<uint8_t>> FileReader::read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::warn("Failed to open file: {}", path);
        return std::nullopt;
    }

    const auto size = file.tellg();
    if (size == -1) {
        spdlog::error("Failed to get file size: {}", path);
        return std::nullopt;
    }

    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        spdlog::error("Failed to read file: {}", path);
        return std::nullopt;
    }

    spdlog::debug("Read file: {} ({} bytes)", path, buffer.size());
    return buffer;
}

std::optional<size_t> FileReader::get_file_size(const std::string& path) {
    std::error_code ec;
    if (std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec)) {
        auto size = std::filesystem::file_size(path, ec);
        if (!ec) {
            return size;
        }
    }

    spdlog::warn("Failed to get file size for {}: {}", path, ec.message());
    return std::nullopt;
}

bool FileReader::is_readable(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }

    // Check if it's a regular file
    if (!S_ISREG(st.st_mode)) {
        return false;
    }

    // Check if readable
    return access(path.c_str(), R_OK) == 0;
}

std::optional<std::string> FileReader::sanitize_path(const std::string& base_dir,
                                                     const std::string& requested_path) {
    try {
        namespace fs = std::filesystem;

        // Resolve base directory; it must exist and be a directory
        auto base_abs = fs::absolute(base_dir);
        if (!fs::exists(base_abs) || !fs::is_directory(base_abs)) {
            spdlog::error("Path error: base directory does not exist or is not a directory: {}",
                          base_dir);
            return std::nullopt;
        }
        auto base = fs::weakly_canonical(base_abs);

        // Combine base with requested path; if requested_path is absolute, it supersedes base
        fs::path requested(requested_path);
        fs::path combined = requested.is_absolute() ? requested : (base / requested);

        // Normalize the combined path (resolve .. and . where possible without requiring the leaf)
        auto full = fs::weakly_canonical(combined);

        // Ensure the resolved path is within the base directory by comparing path components
        auto base_it = base.begin();
        auto full_it = full.begin();
        for (; base_it != base.end() && full_it != full.end(); ++base_it, ++full_it) {
            if (*base_it != *full_it) {
                spdlog::warn("Directory traversal attempt blocked: {} (base: {})", requested_path,
                             base_dir);
                return std::nullopt;
            }
        }
        // If base has more components than full, full cannot be inside base
        if (base_it != base.end()) {
            spdlog::warn("Directory traversal attempt blocked: {} (base: {})", requested_path,
                         base_dir);
            return std::nullopt;
        }

        return full.string();

    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Path error: {}", e.what());
        return std::nullopt;
    }
}

}  // namespace ion
