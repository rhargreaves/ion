#include <spdlog/spdlog.h>

#include <chrono>
#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "curl_client.h"
#include "http2_server.h"
#include "http_response.h"
#include "test_server_runner.h"
#include "wait_for_port.h"

static constexpr uint16_t TEST_PORT = 8443;

inline ion::Http2Server create_test_server() {
    spdlog::set_level(spdlog::level::debug);
    return ion::Http2Server{};
}

TEST_CASE("server: returns static code") {
    auto server = create_test_server();

    server.router().add_route(
        "/", "GET", []() -> ion::HttpResponse { return ion::HttpResponse{.status_code = 200}; });
    TestServerRunner run(server, TEST_PORT);

    CurlClient client;
    const auto res = client.get(std::format("https://localhost:{}/", TEST_PORT));
    REQUIRE(res.status_code == 200);
}

TEST_CASE("server: returns custom headers") {
    auto server = create_test_server();

    server.router().add_route("/hdrs", "GET", []() -> ion::HttpResponse {
        auto resp = ion::HttpResponse{.status_code = 200};
        resp.headers.push_back({"x-foo", "bar"});
        return resp;
    });
    TestServerRunner run(server, TEST_PORT);

    CurlClient client;
    const auto res = client.get(std::format("https://localhost:{}/hdrs", TEST_PORT));
    REQUIRE(res.status_code == 200);
    REQUIRE(res.headers.size() == 2);
    REQUIRE(res.headers.at("x-foo") == "bar");
}

TEST_CASE("server: returns body") {
    auto server = create_test_server();

    server.router().add_route("/body", "GET", [] {
        const std::string body_text = "hello";
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
