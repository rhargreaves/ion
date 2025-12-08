#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "catch2/catch_test_macros.hpp"
#include "curl_client.h"
#include "http2_server.h"
#include "http_response.h"
#include "wait_for_port.h"

static constexpr uint16_t TEST_PORT = 8443;

TEST_CASE("server: test basic handlers") {
    spdlog::set_level(spdlog::level::debug);

    auto server = ion::Http2Server{};

    server.router().register_handler(
        "/", "GET", []() -> ion::HttpResponse { return ion::HttpResponse{.status_code = 200}; });

    server.router().register_handler("/hdrs", "GET", []() -> ion::HttpResponse {
        auto resp = ion::HttpResponse{.status_code = 200};
        resp.headers.push_back({"x-foo", "bar"});
        return resp;
    });

    server.router().register_handler("/body", "GET", []() -> ion::HttpResponse {
        const std::string body_text = "hello";
        const std::vector<uint8_t> body_bytes(body_text.begin(), body_text.end());

        return ion::HttpResponse{.status_code = 200, .body = body_bytes};
    });

    std::thread server_thread([&server]() { server.run_server(TEST_PORT); });

    SECTION ("return matched handler") {
        REQUIRE(wait_for_port(TEST_PORT));

        auto cleanup = [&]() {
            server.stop_server();
            REQUIRE(server_thread.joinable());
            server_thread.join();
        };

        try {
            CurlClient client;
            const long status = client.get(std::format("https://localhost:{}/", TEST_PORT));
            REQUIRE(status == 200);

        } catch (std::exception&) {
            cleanup();
            throw;
        }
        cleanup();
    }

    SECTION ("returns custom headers") {
        REQUIRE(wait_for_port(TEST_PORT));

        auto cleanup = [&]() {
            server.stop_server();
            REQUIRE(server_thread.joinable());
            server_thread.join();
        };

        try {
            CurlClient client;
            const long status = client.get(std::format("https://localhost:{}/hdrs", TEST_PORT));
            REQUIRE(status == 200);

            const auto hdrs = client.headers();
            REQUIRE(hdrs.size() == 2);
            REQUIRE(hdrs.at("x-foo") == "bar");
        } catch (std::exception&) {
            cleanup();
            throw;
        }
        cleanup();
    }

    // SECTION ("returns body") {
    //     REQUIRE(wait_for_port(TEST_PORT));
    //
    //     auto cleanup = [&]() {
    //         server.stop_server();
    //         REQUIRE(server_thread.joinable());
    //         server_thread.join();
    //     };
    //
    //     try {
    //         CurlClient client;
    //         const long status = client.get(std::format("https://localhost:{}/body", TEST_PORT));
    //
    //         REQUIRE(status == 200);
    //         REQUIRE(client.body() == "hello");
    //
    //     } catch (std::exception&) {
    //         cleanup();
    //         throw;
    //     }
    //     cleanup();
    // }
}
