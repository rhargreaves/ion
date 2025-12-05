#pragma once
#include <functional>
#include <string>

#include "http_response.h"

namespace ion {

struct Route {
    std::string path;
    std::string method;
    std::function<HttpResponse()> handler;
};

class Router {
   public:
    std::function<HttpResponse()> get_handler(const std::string& path,
                                              const std::string& method) const;
    void register_handler(const std::string& path, const std::string& method,
                          const std::function<HttpResponse()>& handler);

    Router();

   private:
    std::vector<Route> routes_;
    std::function<HttpResponse()> default_handler_;
};

}  // namespace ion
