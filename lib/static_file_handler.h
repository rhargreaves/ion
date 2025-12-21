#pragma once
#include <string>

#include "http_response.h"

namespace ion {

class StaticFileHandler {
   public:
    StaticFileHandler(const std::string& url_prefix, const std::string& filesystem_root);
    bool matches(const std::string& path) const;
    HttpResponse handle(const std::string& path) const;

   private:
    std::string url_prefix_;
    std::string filesystem_root_;

    std::string get_relative_path(const std::string& url_path) const;
};

}  // namespace ion
