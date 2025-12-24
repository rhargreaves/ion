#pragma once
#include <functional>
#include <string>

#include "http_response.h"
#include "static_file_handler.h"

namespace ion {

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

struct Route {
    std::string path;
    std::string method;
    RouteHandler handler;
};

class Router {
   public:
    Router();
    RouteHandler get_handler(const std::string& path, const std::string& method) const;
    void add_route(const std::string& path, const std::string& method, const RouteHandler& handler);
    void add_static_handler(std::unique_ptr<StaticFileHandler> handler);

   private:
    std::vector<Route> routes_{};
    std::vector<std::unique_ptr<StaticFileHandler>> static_handlers_;
    RouteHandler default_handler_;
};

}  // namespace ion
