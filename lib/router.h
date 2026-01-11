#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "http_response.h"
#include "static_file_handler.h"

namespace ion {

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;
using Middleware = std::function<RouteHandler(RouteHandler)>;

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
    void add_middleware(Middleware mw);

   private:
    std::vector<Route> routes_{};
    std::vector<std::unique_ptr<StaticFileHandler>> static_handlers_;
    RouteHandler default_handler_;
    Middleware middleware_chain_ = [](auto handler) { return handler; };
};

}  // namespace ion
