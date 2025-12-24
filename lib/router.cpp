#include "router.h"

namespace ion {

Router::Router() {
    default_handler_ = [](auto&) { return HttpResponse{404}; };
}

RouteHandler Router::get_handler(const std::string& path, const std::string& method) const {
    RouteHandler target = default_handler_;

    for (const auto& route : routes_) {
        if (route.path == path && route.method == method) {
            target = route.handler;
            break;
        }
    }

    if (method == "GET") {
        for (const auto& handler : static_handlers_) {
            if (handler->matches(path)) {
                target = [&handler, path](const HttpRequest&) -> HttpResponse {
                    return handler->handle(path);
                };
            }
        }
    }

    return middleware_chain_(std::move(target));
}

void Router::add_route(const std::string& path, const std::string& method,
                       const RouteHandler& handler) {
    const Route route{path, method, handler};
    routes_.push_back(route);
}

void Router::add_static_handler(std::unique_ptr<StaticFileHandler> handler) {
    static_handlers_.push_back(std::move(handler));
}

void Router::add_middleware(Middleware mw) {
    auto current_chain = middleware_chain_;
    middleware_chain_ = [current_chain, mw](RouteHandler h) {
        return current_chain(mw(std::move(h)));
    };
}

}  // namespace ion
