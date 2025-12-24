#include "router.h"

namespace ion {

Router::Router() {
    default_handler_ = [](auto&) { return HttpResponse{404}; };
}

RouteHandler Router::get_handler(const std::string& path, const std::string& method) const {
    for (const auto& route : routes_) {
        if (route.path == path && route.method == method) {
            return route.handler;
        }
    }

    if (method == "GET") {
        for (const auto& handler : static_handlers_) {
            if (handler->matches(path)) {
                return [&handler, path](const HttpRequest&) -> HttpResponse {
                    return handler->handle(path);
                };
            }
        }
    }

    return default_handler_;
}

void Router::add_route(const std::string& path, const std::string& method,
                       const RouteHandler& handler) {
    const Route route{path, method, handler};
    routes_.push_back(route);
}

void Router::add_static_handler(std::unique_ptr<StaticFileHandler> handler) {
    static_handlers_.push_back(std::move(handler));
}

}  // namespace ion
