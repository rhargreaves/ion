#pragma once
#include <string>

#include "http_response.h"

namespace ion {

class StaticFileHandler {
   public:
    StaticFileHandler(const std::string& url_prefix, const std::string& filesystem_root);
    bool matches(const std::string& path) const;
    HttpResponse handle(const std::string& path, bool is_head) const;
    static HttpResponse file_response(const std::string& path, bool head_request);

   private:
    std::string url_prefix_;
    std::string filesystem_root_;

    std::string get_relative_path(const std::string& url_path) const;
    static HttpResponse get_response(const std::string& path, const std::string& mime_type);
    static HttpResponse head_response(const std::string& path, const std::string& mime_type);
};

}  // namespace ion
