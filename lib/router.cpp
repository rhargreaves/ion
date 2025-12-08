#include "router.h"

namespace ion {

Router::Router() {
    default_handler_ = [] { return HttpResponse{404}; };
}

std::function<HttpResponse()> Router::get_handler(const std::string& path,
                                                  const std::string& method) const {
    for (const auto& route : routes_) {
        if (route.path == path && route.method == method) {
            return route.handler;
        }
    }
    return default_handler_;
}

void Router::register_handler(const std::string& path, const std::string& method,
                              const std::function<HttpResponse()>& handler) {
    const Route route{path, method, handler};
    routes_.push_back(route);
}

}  // namespace ion
