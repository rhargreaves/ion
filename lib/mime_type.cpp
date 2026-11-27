#include "mime_type.h"

#include <unordered_map>

namespace ion {

std::string MimeType::get_mime_type(const std::string& path) {
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

}  // namespace ion
