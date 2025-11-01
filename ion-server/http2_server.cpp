#include "http2_server.h"

#include "http2_except.h"
#include "spdlog/spdlog.h"
#include "tls_conn.h"
#include "tls_conn_except.h"

enum class Http2ConnectionState { Preface, Frames };

bool process_state(Http2ConnectionState state, Http2Connection& http) {
    switch (state) {
        case Http2ConnectionState::Preface: {
            if (http.try_read_preface()) {
                return true;
            }
            return false;
        }
        case Http2ConnectionState::Frames: {
            if (http.try_read_frame()) {
                spdlog::debug("Frame processed, continuing...");
                return true;
            }
            return false;
        }
        default: {
            throw std::runtime_error("Invalid state");
        }
    }
}

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

            auto connection_start = std::chrono::steady_clock::now();
            constexpr auto connection_timeout = std::chrono::seconds(10);

            Http2Connection http{tls_conn};
            Http2ConnectionState state = Http2ConnectionState::Preface;

            while (!should_stop_) {
                if (process_state(state, http)) {
                    connection_start = std::chrono::steady_clock::now();

                    if (state == Http2ConnectionState::Preface) {
                        state = Http2ConnectionState::Frames;
                    }
                }

                if (std::chrono::steady_clock::now() - connection_start > connection_timeout) {
                    spdlog::debug("Connection timeout, closing");
                    break;
                }
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
