#include "static_file_handler.h"

#include <spdlog/spdlog.h>

#include "file_reader.h"
#include "http_response.h"

namespace ion {

StaticFileHandler::StaticFileHandler(const std::string& url_prefix,
                                     const std::string& filesystem_root)
    : url_prefix_(url_prefix), filesystem_root_(filesystem_root) {
    spdlog::info("StaticFileHandler created: '{}' -> '{}'", url_prefix_, filesystem_root_);
}

bool StaticFileHandler::matches(const std::string& path) const {
    return path.starts_with(url_prefix_);
}

std::string StaticFileHandler::get_relative_path(const std::string& url_path) const {
    std::string rel_path = url_path.substr(url_prefix_.length());

    if (rel_path.empty() || rel_path == "/") {
        rel_path = "/index.html";
    }

    if (rel_path.starts_with("/")) {
        rel_path = rel_path.substr(1);
    }

    // remove query string
    if (const auto query_pos = rel_path.find('?'); query_pos != std::string::npos) {
        rel_path = rel_path.substr(0, query_pos);
    }

    return rel_path;
}

HttpResponse StaticFileHandler::handle(const std::string& path) const {
    std::string rel_path = get_relative_path(path);

    auto safe_path = FileReader::sanitize_path(filesystem_root_, rel_path);
    if (!safe_path) {
        spdlog::warn("Invalid file path requested: {}", path);
        return HttpResponse{403};  // Forbidden
    }

    if (!FileReader::is_readable(*safe_path)) {
        spdlog::debug("File not found or not readable: {}", *safe_path);
        return HttpResponse{404};
    }

    auto content = FileReader::read_file(*safe_path);
    if (!content) {
        spdlog::error("Failed to read file: {}", *safe_path);
        return HttpResponse{500};
    }

    auto mime_type = FileReader::get_mime_type(*safe_path);
    spdlog::info("Serving static file: {} ({} bytes, {})", *safe_path, content->size(), mime_type);

    return HttpResponse{
        .status_code = 200, .body = std::move(*content), .headers = {{"content-type", mime_type}}};
}

}  // namespace ion
