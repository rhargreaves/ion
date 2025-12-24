#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "curl_client.h"
#include "http2_server.h"
#include "static_file_handler.h"
#include "test_helpers.h"
#include "test_server_runner.h"

static constexpr uint16_t TEST_PORT = 8443;

TEST_CASE("static: basic static file mapping") {
    auto server = TestHelpers::create_test_server();

    auto sfh = std::make_unique<ion::StaticFileHandler>("/static", "./test/system/static");

    server.router().add_static_handler(std::move(sfh));

    TestServerRunner run(server, TEST_PORT);

    CurlClient client;

    SECTION ("serves index.html explicitly") {
        const auto res =
            client.get(std::format("https://localhost:{}/static/index.html", TEST_PORT));
        REQUIRE(res.status_code == 200);
        REQUIRE(res.headers.contains("content-type"));
        CHECK(res.headers.at("content-type").starts_with("text/html"));
        CHECK(res.body.find("Hello from static") != std::string::npos);
    }

    SECTION ("maps /static to index.html") {
        const auto res = client.get(std::format("https://localhost:{}/static", TEST_PORT));
        REQUIRE(res.status_code == 200);
        CHECK(res.body.find("Hello from static") != std::string::npos);
    }

    SECTION ("maps /static/ to index.html") {
        const auto res = client.get(std::format("https://localhost:{}/static/", TEST_PORT));
        REQUIRE(res.status_code == 200);
        CHECK(res.body.find("Hello from static") != std::string::npos);
    }

    SECTION ("returns 404 for missing file") {
        const auto res =
            client.get(std::format("https://localhost:{}/static/missing.txt", TEST_PORT));
        REQUIRE(res.status_code == 404);
    }

    SECTION ("blocks directory traversal attempts") {
        const auto res =
            client.get(std::format("https://localhost:{}/static/../secret.txt", TEST_PORT));
        REQUIRE((res.status_code == 403 || res.status_code == 404));
    }

    SECTION ("strips query string") {
        const auto res =
            client.get(std::format("https://localhost:{}/static/index.html?v=1", TEST_PORT));
        REQUIRE(res.status_code == 200);
    }
}

TEST_CASE("static: middleware support") {
    auto server = TestHelpers::create_test_server();

    auto sfh = std::make_unique<ion::StaticFileHandler>("/static", "./test/system/static");

    auto& router = server.router();

    router.add_middleware([](auto next) {
        return [next](const auto& req) {
            auto res = next(req);
            if (res.status_code == 404) {
                res.status_code = 202;
            }
            return res;
        };
    });

    router.add_static_handler(std::move(sfh));

    TestServerRunner run(server, TEST_PORT);

    CurlClient client;

    SECTION ("intercepts static file not found error") {
        const auto res =
            client.get(std::format("https://localhost:{}/static/missing.txt", TEST_PORT));
        REQUIRE(res.status_code == 202);
    }
}
