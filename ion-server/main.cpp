#include <csignal>
#include <iostream>
#include <vector>

#include "http2_conn.h"
#include "http2_except.h"
#include "http2_frames.h"
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
    std::cout << "Signal received!..." << std::endl;
    should_stop = 1;
}

void write_response(Http2Connection& http) {
    // Send 200 response: HPACK index 8 = ":status: 200"
    std::array<uint8_t, 1> headers_data = {0x88};  // 0x80 | 8 = indexed header
    http.write_headers_response(1, headers_data, FLAG_END_HEADERS | FLAG_END_STREAM);
    std::cout << "200 response sent" << std::endl;
}

void read_frame(Http2Connection& http) {
    const auto header = http.read_frame_header();

    switch (header.type) {
        case FRAME_TYPE_SETTINGS: {
            std::cout << "SETTINGS frame received" << std::endl;
            const auto settings = http.read_settings_payload(header.length);
            std::cout << " - Received " << settings.size() << " settings" << std::endl;

            if (header.flags == 0x00) {
                http.write_settings_ack();
                std::cout << "SETTINGS ACK frame sent" << std::endl;
            }
            break;
        }
        case FRAME_TYPE_WINDOW_UPDATE: {
            std::cout << "WINDOW_UPDATE frame received" << std::endl;
            auto [window_size_increment] = http.read_window_update(header.length);
            std::cout << " - Window size increment: " << window_size_increment << std::endl;
            break;
        }
        case FRAME_TYPE_HEADERS: {
            std::cout << "HEADERS frame received" << std::endl;
            auto payload = http.read_payload(header.length);
            const bool end_headers_set = (header.flags & FLAG_END_HEADERS) != 0;
            const bool end_stream_set = (header.flags & FLAG_END_STREAM) != 0;
            std::cout << " - Stream ID: " << header.stream_id << std::endl;
            std::cout << " - End headers: " << end_headers_set << std::endl;
            std::cout << " - End stream: " << end_stream_set << std::endl;
            std::cout << " - Payload size: " << header.length << std::endl;
            write_response(http);
            break;
        }
        default: {
            std::cout << "Received frame of type " << +header.type << std::endl;
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
    std::cout << "SETTINGS frame sent" << std::endl;
}

void run_server() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    TcpConnection tcp_conn{SERVER_PORT};
    tcp_conn.listen();
    std::cout << "[ion] Listening on port " << SERVER_PORT << "..." << std::endl;

    while (!should_stop) {
        try {
            tcp_conn.accept();
            std::cout << "Client connected." << std::endl;

            TlsConnection tls_conn{tcp_conn, "cert.pem", "key.pem"};
            tls_conn.handshake();
            std::cout << "SSL handshake completed successfully." << std::endl;

            Http2Connection http{tls_conn};
            http.read_preface();
            std::cout << "Valid HTTP/2 preface received!" << std::endl;

            write_settings(http);

            while (tls_conn.has_data() && !should_stop) {
                read_frame(http);
            }

            http.write_goaway(1);
            std::cout << "GOAWAY frame sent" << std::endl;

            if (!http.wait_for_client_disconnect()) {
                std::cout << "Client took too long to disconnect. Forcibly closing" << std::endl;
            }
            std::cout << "Connection closed." << std::endl;

        } catch (const TlsConnectionClosed& e) {
            std::cerr << "Connection closed (exception)" << e.what() << std::endl;
        } catch (const Http2TimeoutException& e) {
            std::cerr << "Connection closed (timeout)" << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            break;
        }
    }

    if (should_stop) {
        std::cout << "Server shutting down..." << std::endl;
    }
}

int main() {
    try {
        run_server();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ion] " << e.what() << std::endl;
        return 1;
    }
}
