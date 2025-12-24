#include "catch2/catch_test_macros.hpp"
#include "router.h"

TEST_CASE("router: middleware support") {
    auto router = ion::Router{};
    auto dummy_req = ion::HttpRequest{};

    SECTION ("manipulates added route") {
        router.add_route("/foo", "GET",
                         [](auto&) { return ion::HttpResponse{.status_code = 404}; });

        router.add_middleware([](auto next) {
            return [next](const auto& req) {
                auto res = next(req);
                if (res.status_code == 404) {
                    res.status_code = 202;
                }
                return res;
            };
        });

        auto handler = router.get_handler("/bar", "GET");

        REQUIRE(handler(dummy_req).status_code == 202);
    }

    SECTION ("manipulates default handler") {
        router.add_middleware([](auto next) {
            return [next](const auto& req) {
                auto res = next(req);
                if (res.status_code == 404) {
                    res.status_code = 201;
                }
                return res;
            };
        });

        auto handler = router.get_handler("/bar", "GET");

        REQUIRE(handler(dummy_req).status_code == 201);
    }
}
