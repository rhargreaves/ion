#pragma once
#include <memory>
#include <string>

#include "file_store.h"
#include "http_response.h"

namespace ion {

class StaticFileHandler {
   public:
    StaticFileHandler(const std::string& url_prefix, const std::string& filesystem_root,
                      std::unique_ptr<FileStore> store);
    bool matches(const std::string& path) const;
    HttpResponse handle(const std::string& path, bool is_head) const;
    HttpResponse file_response(const std::string& path, bool head_request) const;

   private:
    std::string url_prefix_;
    std::string filesystem_root_;
    std::unique_ptr<FileStore> store_;

    std::string get_relative_path(const std::string& url_path) const;
    HttpResponse get_response(const std::string& path, const std::string& mime_type) const;
    HttpResponse head_response(const std::string& path, const std::string& mime_type) const;
};

}  // namespace ion
