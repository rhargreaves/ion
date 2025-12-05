#include "catch2/catch_test_macros.hpp"
#include "router.h"

TEST_CASE("router: returns appropriate handler function") {
    auto router = ion::Router{};

    SECTION ("return default handler if no handlers defined") {
        auto handler = router.get_handler("/", "GET");

        REQUIRE(handler().status_code == 404);
    }

    SECTION ("return default handler if non-matched") {
        router.register_handler("/foo", "GET",
                                []() { return ion::HttpResponse{.status_code = 200}; });

        auto handler = router.get_handler("/", "GET");

        REQUIRE(handler().status_code == 404);
    }

    SECTION ("return matched handler") {
        router.register_handler("/foo", "GET",
                                []() { return ion::HttpResponse{.status_code = 200}; });
        router.register_handler("/bar", "POST",
                                []() { return ion::HttpResponse{.status_code = 201}; });
        router.register_handler("/bar", "GET",
                                []() { return ion::HttpResponse{.status_code = 202}; });
        router.register_handler("/baz", "GET",
                                []() { return ion::HttpResponse{.status_code = 203}; });

        auto handler = router.get_handler("/bar", "GET");

        REQUIRE(handler().status_code == 202);
    }
}
