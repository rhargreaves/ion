#include "catch2/catch_test_macros.hpp"
#include "router.h"

TEST_CASE("router: returns appropriate handler function") {
    auto router = ion::Router{};

    SECTION ("return default handler") {
        auto handler = router.get_handler("dummy", "dummy");

        REQUIRE(handler().status_code == 404);
    }

    SECTION ("return last handler added") {
        router.register_handler([]() { return HttpResponse{.status_code = 200}; });

        auto handler = router.get_handler("dummy", "dummy");

        REQUIRE(handler().status_code == 200);
    }
}
