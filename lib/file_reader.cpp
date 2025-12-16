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

    auto size = file.tellg();
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

std::string FileReader::get_mime_type(const std::string& path) {
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".txt", "text/plain; charset=utf-8"},
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".webp", "image/webp"},
    };

    auto pos = path.find_last_of('.');
    if (pos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = path.substr(pos);
    auto it = mime_types.find(ext);
    return it != mime_types.end() ? it->second : "application/octet-stream";
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

        // Resolve base directory to canonical path
        auto base = fs::canonical(fs::absolute(base_dir));
        // Construct full path (don't canonicalize yet - file might not exist)
        auto full_path = fs::absolute(base / requested_path);

        // Check if the path starts with base_dir (prevents traversal)
        auto full_str = full_path.string();
        auto base_str = base.string();

        if (full_str.find(base_str) != 0) {
            spdlog::warn("Directory traversal attempt blocked: {} (base: {})", requested_path,
                         base_dir);
            return std::nullopt;
        }

        return full_str;

    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Path error: {}", e.what());
        return std::nullopt;
    }
}

}  // namespace ion
