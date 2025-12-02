#pragma once
#include <functional>
#include <string>

#include "http_response.h"

namespace ion {

class Router {
   public:
    std::function<HttpResponse()> get_handler(const std::string& path, const std::string& method);
    void register_handler(const std::function<HttpResponse()>& handler);

    Router();

   private:
    std::vector<std::function<HttpResponse()>> routes_;
};

}  // namespace ion
