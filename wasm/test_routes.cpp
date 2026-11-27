#include "test_routes.h"

namespace ion::wasm {

void TestRoutes::add_test_routes(Router& router) {
    router.add_route("/_tests/ok", "GET",
                     [](const auto&) { return HttpResponse{.status_code = 200}; });

    router.add_route("/_tests/no_content", "GET",
                     [](const auto&) { return HttpResponse{.status_code = 204}; });

    router.add_route("/_tests/medium_body", "GET", [](const auto&) {
        constexpr size_t content_size = 128 * 1024;
        std::string body(content_size, 'A');

        return HttpResponse{.status_code = 200,
                            .body = std::vector<uint8_t>{body.begin(), body.end()}};
    });

    router.add_route("/_tests/large_body", "GET", [](const auto&) {
        constexpr size_t content_size = 2 * 1024 * 1024;
        std::string body(content_size, 'A');

        return HttpResponse{.status_code = 200,
                            .body = std::vector<uint8_t>{body.begin(), body.end()}};
    });
}

}  // namespace ion::wasm
