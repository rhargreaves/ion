#include <atomic>
#include <chrono>
#include <thread>

#include "catch2/catch_test_macros.hpp"
#include "curl_client.h"
#include "http2_server.h"
#include "http_response.h"
#include "wait_for_port.h"

static constexpr uint16_t TEST_PORT = 8081;

TEST_CASE("server: test basic handlers") {
    auto server = ion::Http2Server{};

    server.router().register_handler(
        "/", "GET", []() -> ion::HttpResponse { return ion::HttpResponse{.status_code = 200}; });

    SECTION ("return matched handler") {
        std::thread server_thread([&server]() { server.run_server(TEST_PORT); });

        REQUIRE(wait_for_port(TEST_PORT));

        try {
            CurlClient client;
            const long status = client.get(std::format("https://localhost:{}/", TEST_PORT));
            REQUIRE(status == 200);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            server.stop_server();
            if (server_thread.joinable()) {
                server_thread.join();
            }

        } catch (std::exception&) {
            server.stop_server();
            if (server_thread.joinable()) {
                server_thread.join();
            }
            throw;
        }
    }
}
