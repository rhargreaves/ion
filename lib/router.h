#pragma once
#include <functional>
#include <string>

#include "http_response.h"
#include "static_file_handler.h"

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
    void add_route(const std::string& path, const std::string& method,
                   const std::function<HttpResponse()>& handler);

    void add_static_handler(std::unique_ptr<StaticFileHandler> handler);

    Router();

   private:
    std::vector<Route> routes_{};
    std::vector<std::unique_ptr<StaticFileHandler>> static_handlers_;
    std::function<HttpResponse()> default_handler_;
};

}  // namespace ion
