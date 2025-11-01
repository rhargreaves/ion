#include <csignal>
#include <vector>

#include "http2_conn.h"
#include "http2_except.h"
#include "http2_frames.h"
#include "spdlog/spdlog.h"
#include "tls_conn.h"
#include "tls_conn_except.h"

static constexpr uint16_t SERVER_PORT = 8443;

static volatile sig_atomic_t should_stop = 0;

void signal_handler(int) {
    spdlog::info("signal received!...");
    should_stop = 1;
}

void write_settings(Http2Connection& http) {
    const std::vector<Http2Setting> settings = {
        {0x0003, 100},    // MAX_CONCURRENT_STREAMS
        {0x0004, 65535},  // INITIAL_WINDOW_SIZE
        {0x0005, 16384}   // MAX_FRAME_SIZE
    };
    http.write_settings(settings);
    spdlog::debug("SETTINGS frame sent");
}

void run_server() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    TcpConnection tcp_conn{SERVER_PORT};
    tcp_conn.listen();
    spdlog::info("listening on port {}", SERVER_PORT);

    while (!should_stop) {
        try {
            if (!tcp_conn.try_accept()) {
                continue;
            }
            spdlog::info("client connected");

            TlsConnection tls_conn{tcp_conn, "cert.pem", "key.pem"};
            tls_conn.handshake();
            spdlog::info("SSL handshake completed successfully");

            Http2Connection http{tls_conn};
            http.read_preface();
            spdlog::info("valid HTTP/2 preface received!");

            write_settings(http);

            auto connection_start = std::chrono::steady_clock::now();
            constexpr auto connection_timeout = std::chrono::seconds(10);

            while (!should_stop) {
                if (http.try_read_frame()) {
                    spdlog::debug("Frame processed, continuing...");
                    connection_start = std::chrono::steady_clock::now();
                    continue;
                }

                if (std::chrono::steady_clock::now() - connection_start > connection_timeout) {
                    spdlog::debug("Connection timeout, closing");
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            http.write_goaway(1);
            spdlog::debug("GOAWAY frame sent");

            tls_conn.graceful_shutdown();
        } catch (const TlsConnectionClosed& e) {
            spdlog::info("connection closed (exception): {}", e.what());
        } catch (const Http2TimeoutException& e) {
            spdlog::info("connection closed (timeout): {}", e.what());
        } catch (const std::exception& e) {
            spdlog::error(e.what());
            throw;
        }
    }

    if (should_stop) {
        spdlog::info("server shutting down...");
    }
}

int main() {
    spdlog::info("ion started ⚡️");
    spdlog::set_level(spdlog::level::debug);

    try {
        run_server();
        spdlog::info("exiting");
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("terminating: {}", e.what());
        return 1;
    }
}
