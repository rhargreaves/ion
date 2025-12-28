

#include "test_routes.h"

#include "proc_ctrl.h"

void TestRoutes::add_test_routes(ion::Router& router) {
    router.add_route("/_tests/ok", "GET",
                     [](const auto&) { return ion::HttpResponse{.status_code = 200}; });

    router.add_route("/_tests/no_content", "GET",
                     [](const auto&) { return ion::HttpResponse{.status_code = 204}; });

    router.add_route("/_tests/no_new_privs", "GET", [](const auto&) {
        if (ProcessControl::check_no_new_privs()) {
            return ion::HttpResponse{.status_code = 200};
        }
        return ion::HttpResponse{.status_code = 500};
    });

    router.add_route("/_tests/medium_body", "GET", [](const auto&) {
        constexpr size_t content_size = 128 * 1024;
        std::string body(content_size, 'A');

        return ion::HttpResponse{.status_code = 200,
                                 .body = std::vector<uint8_t>{body.begin(), body.end()}};
    });

    router.add_route("/_tests/large_body", "GET", [](const auto&) {
        constexpr size_t content_size = 2 * 1024 * 1024;
        std::string body(content_size, 'A');

        return ion::HttpResponse{.status_code = 200,
                                 .body = std::vector<uint8_t>{body.begin(), body.end()}};
    });
}
