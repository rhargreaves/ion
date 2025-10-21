#include <iostream>
#include <vector>

#include "http2_connection.h"
#include "http2_frames.h"
#include "tls_conn.h"

static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint8_t FLAG_END_HEADERS = 0x04;
static constexpr uint8_t FLAG_END_STREAM = 0x01;

static constexpr uint16_t SERVER_PORT = 8443;

void read_frame(Http2Connection& http2) {
    const auto header = http2.read_frame_header();

    switch (header.type) {
        case FRAME_TYPE_SETTINGS: {
            std::cout << "SETTINGS frame received" << std::endl;
            auto settings = http2.read_settings_payload(header.length);
            std::cout << "Received " << settings.size() << " settings" << std::endl;
            break;
        }
        default: {
            std::cout << "Received frame of type " << +header.type << std::endl;
            auto data = http2.read_payload(header.length);  // Read and discard
            break;
        }
    }
}

void run_server() {
    TlsConnection tls_conn{SERVER_PORT};
    tls_conn.listen();
    std::cout << "[ion] Listening on port " << SERVER_PORT << "..." << std::endl;

    tls_conn.accept();
    std::cout << "Client connected." << std::endl;

    tls_conn.handshake("cert.pem", "key.pem");
    std::cout << "SSL handshake completed successfully." << std::endl;

    Http2Connection http2{tls_conn};

    http2.read_preface();
    std::cout << "Valid HTTP/2 preface received!" << std::endl;

    read_frame(http2);

    const std::vector<Http2Setting> settings = {
        {0x0003, 100},    // MAX_CONCURRENT_STREAMS
        {0x0004, 65535},  // INITIAL_WINDOW_SIZE
        {0x0005, 16384}   // MAX_FRAME_SIZE
    };
    http2.write_settings(settings);
    std::cout << "SETTINGS frame sent" << std::endl;

    read_frame(http2);

    // Send 200 response: HPACK index 8 = ":status: 200"
    std::array<uint8_t, 1> headers_data = {0x88};  // 0x80 | 8 = indexed header
    http2.write_headers_response(1, headers_data, FLAG_END_HEADERS | FLAG_END_STREAM);
    std::cout << "200 response sent" << std::endl;

    http2.write_goaway(1);
    std::cout << "GOAWAY frame sent" << std::endl;

    tls_conn.close();
    std::cout << "Connection closed." << std::endl;
}

int main() {
    try {
        run_server();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
