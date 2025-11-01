#include "http2_server.h"

#include "http2_except.h"
#include "http2_frames.h"
#include "spdlog/spdlog.h"
#include "tls_conn.h"
#include "tls_conn_except.h"

void Http2Server::run_server(uint16_t port) {
    TcpConnection tcp_conn{port};
    tcp_conn.listen();
    spdlog::info("listening on port {}", port);

    while (!should_stop_) {
        try {
            if (!tcp_conn.try_accept()) {
                continue;
            }
            spdlog::info("client connected");

            TlsConnection tls_conn{tcp_conn, "cert.pem", "key.pem"};
            tls_conn.handshake();
            spdlog::info("SSL handshake completed successfully");

            Http2Connection http{tls_conn};
            while (!should_stop_) {
                if (http.try_read_preface()) {
                    spdlog::info("valid HTTP/2 preface received!");
                    break;
                }
            }
            http.write_settings();

            auto connection_start = std::chrono::steady_clock::now();
            constexpr auto connection_timeout = std::chrono::seconds(10);

            while (!should_stop_) {
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

    if (should_stop_) {
        spdlog::info("server shutting down...");
    }
}

void Http2Server::stop_server() { should_stop_ = 1; }
