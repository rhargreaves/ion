#include "catch2/catch_test_macros.hpp"
#include "router.h"

TEST_CASE("router: returns appropriate handler function") {
    auto router = ion::Router{};
    auto dummy_req = ion::HttpRequest{};

    SECTION ("return default handler if no handlers defined") {
        auto handler = router.get_handler("/", "GET");

        REQUIRE(handler(dummy_req).status_code == 404);
    }

    SECTION ("return default handler if non-matched") {
        router.add_route("/foo", "GET",
                         [](auto&) { return ion::HttpResponse{.status_code = 200}; });

        auto handler = router.get_handler("/", "GET");

        REQUIRE(handler(dummy_req).status_code == 404);
    }

    SECTION ("return matched handler") {
        router.add_route("/foo", "GET",
                         [](auto&) { return ion::HttpResponse{.status_code = 200}; });
        router.add_route("/bar", "POST",
                         [](auto&) { return ion::HttpResponse{.status_code = 201}; });
        router.add_route("/bar", "GET",
                         [](auto&) { return ion::HttpResponse{.status_code = 202}; });
        router.add_route("/baz", "GET",
                         [](auto&) { return ion::HttpResponse{.status_code = 203}; });

        auto handler = router.get_handler("/bar", "GET");

        REQUIRE(handler(dummy_req).status_code == 202);
    }
}
