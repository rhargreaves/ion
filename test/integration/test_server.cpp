#include <curl/curl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "catch2/catch_test_macros.hpp"
#include "http2_server.h"
#include "http_response.h"

static constexpr uint16_t MAX_PORT_WAIT_ATTEMPTS = 50;
static constexpr uint16_t TEST_PORT = 8081;

bool wait_for_port(uint16_t port) {
    for (int i = 0; i < MAX_PORT_WAIT_ATTEMPTS; i++) {
        const int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
            continue;
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        const bool connected =
            connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0;
        close(sock);

        if (connected)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

class CurlClient {
    CURL* curl_;
    std::string response_body_;

    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
        return size * nmemb;
    }

   public:
    CurlClient() {
        curl_ = curl_easy_init();
        curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body_);
    }

    ~CurlClient() {
        curl_easy_cleanup(curl_);
    }
    long get(const std::string& url) {
        response_body_.clear();
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_perform(curl_);

        long http_code = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
        return http_code;
    }

    const std::string& body() const {
        return response_body_;
    }
};

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
