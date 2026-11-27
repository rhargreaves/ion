#pragma once
#include <string>

namespace ion {

class MimeType {
   public:
    static std::string get_mime_type(const std::string& path);
};

}  // namespace ion
