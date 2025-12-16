#include <spdlog/spdlog.h>

#include <chrono>
#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "curl_client.h"
#include "http2_server.h"
#include "static_file_handler.h"
#include "test_server_runner.h"

static constexpr uint16_t TEST_PORT = 8443;

inline ion::Http2Server create_test_server() {
    spdlog::set_level(spdlog::level::debug);
    return ion::Http2Server{};
}

TEST_CASE("static: returns static content") {
    auto server = create_test_server();

    auto sfh = std::make_unique<ion::StaticFileHandler>("/static", "./test/system/static");

    server.router().add_static_handler(std::move(sfh));

    TestServerRunner run(server, TEST_PORT);

    CurlClient client;
    const auto res = client.get(std::format("https://localhost:{}/static/index.html", TEST_PORT));
    REQUIRE(res.status_code == 200);
}
