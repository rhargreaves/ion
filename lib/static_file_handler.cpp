#include "static_file_handler.h"

#include <spdlog/spdlog.h>

#include "file_reader.h"
#include "http_response.h"

namespace ion {

StaticFileHandler::StaticFileHandler(const std::string& url_prefix,
                                     const std::string& filesystem_root)
    : url_prefix_(url_prefix), filesystem_root_(filesystem_root) {}

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

HttpResponse StaticFileHandler::file_response(const std::string& path, bool is_head) {
    if (!FileReader::is_readable(path)) {
        spdlog::debug("file not found or not readable: {}", path);
        return HttpResponse{404};
    }

    auto mime_type = FileReader::get_mime_type(path);
    if (is_head) {
        auto size = FileReader::get_file_size(path);
        if (!size) {
            spdlog::error("failed to get file size: {}", path);
            return HttpResponse{500};
        }

        spdlog::info("Returning metadata for static file: {} ({} bytes, {})", path, *size,
                     mime_type);
        return HttpResponse{
            .status_code = 200,
            .headers = {{"content-length", std::to_string(*size)}, {"content-type", mime_type}}};
    }

    auto content = FileReader::read_file(path);
    if (!content) {
        spdlog::error("failed to read file: {}", path);
        return HttpResponse{500};
    }

    spdlog::info("serving static file: {} ({} bytes, {})", path, content->size(), mime_type);

    return HttpResponse{
        .status_code = 200, .body = std::move(*content), .headers = {{"content-type", mime_type}}};
}

HttpResponse StaticFileHandler::handle(const std::string& path, bool is_head) const {
    std::string rel_path = get_relative_path(path);

    auto safe_path = FileReader::sanitize_path(filesystem_root_, rel_path);
    if (!safe_path) {
        spdlog::warn("invalid file path requested: {}", path);
        return HttpResponse{403};
    }

    return file_response(*safe_path, is_head);
}

}  // namespace ion
