#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "curl_client.h"
#include "http2_server.h"
#include "http_response.h"
#include "test_helpers.h"
#include "test_server_runner.h"

static constexpr uint16_t TEST_PORT = 8443;

TEST_CASE("server: returns x-powered-by header (frontends overwrite server header)") {
    auto server = TestHelpers::create_test_server();

    TestServerRunner run(server, TEST_PORT);

    CurlClient client;
    const auto res = client.get(std::format("https://localhost:{}/", TEST_PORT));
    REQUIRE(res.status_code == 404);
    REQUIRE(res.headers.at("x-powered-by") == "ion");
}

TEST_CASE("server: returns status code") {
    auto server = TestHelpers::create_test_server();

    server.router().add_route("/", "GET",
                              [](auto&) { return ion::HttpResponse{.status_code = 200}; });
    TestServerRunner run(server, TEST_PORT);

    CurlClient client;
    const auto res = client.get(std::format("https://localhost:{}/", TEST_PORT));
    REQUIRE(res.status_code == 200);
}

TEST_CASE("server: returns custom headers") {
    auto server = TestHelpers::create_test_server();

    server.router().add_route("/hdrs", "GET", [](auto&) {
        auto resp = ion::HttpResponse{.status_code = 200};
        resp.headers.push_back({"x-foo", "bar"});
        return resp;
    });
    TestServerRunner run(server, TEST_PORT);

    CurlClient client;
    const auto res = client.get(std::format("https://localhost:{}/hdrs", TEST_PORT));
    REQUIRE(res.status_code == 200);
    REQUIRE(res.headers.size() == 3);
    REQUIRE(res.headers.at("x-foo") == "bar");
}

TEST_CASE("server: returns body") {
    auto server = TestHelpers::create_test_server();

    server.router().add_route("/body", "GET", [](auto&) {
        constexpr std::string body_text = "hello";
        const std::vector<uint8_t> body_bytes(body_text.begin(), body_text.end());

        return ion::HttpResponse{
            .status_code = 200, .body = body_bytes, .headers = {{"content-type", "text/plain"}}};
    });
    TestServerRunner run(server, TEST_PORT);

    CurlClient client;
    const auto res = client.get(std::format("https://localhost:{}/body", TEST_PORT));
    REQUIRE(res.status_code == 200);
    REQUIRE(res.headers.at("content-type") == "text/plain");
    REQUIRE(res.headers.at("content-length") == "5");
    REQUIRE(res.body == "hello");
}

TEST_CASE("server: validates configuration") {
    SECTION ("throws if TLS cert & key paths missing") {
        REQUIRE_THROWS_AS(ion::Http2Server{ion::ServerConfiguration{}}, std::runtime_error);
    }

    SECTION ("does not throw if TLS cert & key paths provided") {
        REQUIRE_NOTHROW(ion::Http2Server{
            ion::ServerConfiguration{.cert_path = "cert.pem", .key_path = "key.pem"}});
    }

    SECTION ("does not throw if cleartext enabled and TLS cert & key paths missing") {
        REQUIRE_NOTHROW(ion::Http2Server{ion::ServerConfiguration{.cleartext = true}});
    }
}
