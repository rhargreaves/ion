#include "router.h"

namespace ion {

Router::Router() {
    // add default route
    routes_.emplace_back([] { return HttpResponse(404); });
}

std::function<HttpResponse()> Router::get_handler(const std::string& path,
                                                  const std::string& method) {
    return routes_[routes_.size() - 1];
}

void Router::register_handler(const std::function<HttpResponse()>& handler) {
    routes_.push_back(handler);
}

}  // namespace ion
