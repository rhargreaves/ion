#include <csignal>
#include <iostream>
#include <vector>

#include "http2_conn.h"
#include "http2_except.h"
#include "http2_frames.h"
#include "spdlog/spdlog.h"
#include "tls_conn.h"
#include "tls_conn_except.h"

static constexpr uint8_t FRAME_TYPE_HEADERS = 0x01;
static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint8_t FRAME_TYPE_WINDOW_UPDATE = 0x08;
static constexpr uint8_t FLAG_END_HEADERS = 0x04;
static constexpr uint8_t FLAG_END_STREAM = 0x01;

static constexpr uint16_t SERVER_PORT = 8443;

static volatile sig_atomic_t should_stop = 0;

void signal_handler(int) {
    spdlog::info("signal received!...");
    should_stop = 1;
}

void write_response(Http2Connection& http) {
    // Send 200 response: HPACK index 8 = ":status: 200"
    std::array<uint8_t, 1> headers_data = {0x88};  // 0x80 | 8 = indexed header
    http.write_headers_response(1, headers_data, FLAG_END_HEADERS | FLAG_END_STREAM);
    spdlog::info("200 response sent");
}

void read_frame(Http2Connection& http) {
    const auto header = http.read_frame_header();

    switch (header.type) {
        case FRAME_TYPE_SETTINGS: {
            spdlog::debug("SETTINGS frame received");
            const auto settings = http.read_settings_payload(header.length);
            spdlog::debug(" - received {} settings", settings.size());

            if (header.flags == 0x00) {
                http.write_settings_ack();
                spdlog::debug("SETTINGS ACK frame sent");
            }
            break;
        }
        case FRAME_TYPE_WINDOW_UPDATE: {
            spdlog::debug("WINDOW_UPDATE frame received");
            auto [window_size_increment] = http.read_window_update(header.length);
            spdlog::debug(" - window size increment: {}", window_size_increment);
            break;
        }
        case FRAME_TYPE_HEADERS: {
            spdlog::debug("HEADERS frame received");
            auto payload = http.read_payload(header.length);
            const bool end_headers_set = (header.flags & FLAG_END_HEADERS) != 0;
            const bool end_stream_set = (header.flags & FLAG_END_STREAM) != 0;
            spdlog::debug(" - stream ID: {}", header.stream_id);
            spdlog::debug(" - end headers: {}", end_headers_set);
            spdlog::debug(" - end stream: {}", end_stream_set);
            spdlog::debug(" - payload size: {}", header.length);
            write_response(http);
            break;
        }
        default: {
            spdlog::debug("received frame of type {}", +header.type);
            auto data = http.read_payload(header.length);  // Read and discard
            break;
        }
    }
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
    spdlog::info("listening on port {}...", SERVER_PORT);

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

            while (tls_conn.has_data() && !should_stop) {
                read_frame(http);
            }

            http.write_goaway(1);
            spdlog::debug("GOAWAY frame sent");

            if (!http.wait_for_client_disconnect()) {
                spdlog::warn("client took too long to disconnect, forcibly closing");
            }
            spdlog::info("connection closed");

        } catch (const TlsConnectionClosed& e) {
            spdlog::info("connection closed (exception): {}", e.what());
        } catch (const Http2TimeoutException& e) {
            spdlog::info("connection closed (timeout): {}", e.what());
        } catch (const std::exception& e) {
            spdlog::error("{}", e.what());
            break;
        }
    }

    if (should_stop) {
        spdlog::info("server shutting down...");
    }
}

int main() {
    spdlog::info("ion started ⚡️");

    try {
        run_server();
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("[ion] {}", e.what());
        return 1;
    }
}
