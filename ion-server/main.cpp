#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "tls_conn.h"

#pragma pack(push, 1)
struct Http2FrameHeader {
    uint8_t length[3];    // 24-bit length (big-endian)
    uint8_t type;         // Frame type
    uint8_t flags;        // Frame flags
    uint32_t stream_id;   // Stream identifier (big-endian)

    void set_length(uint32_t len) {
        length[0] = (len >> 16) & 0xFF;
        length[1] = (len >> 8) & 0xFF;
        length[2] = len & 0xFF;
    }

    void set_stream_id(uint32_t id) {
        stream_id = htonl(id);
    }
};

struct Http2Setting {
    uint16_t identifier;
    uint32_t value;

    Http2Setting(uint16_t id, uint32_t val)
        : identifier(htons(id)), value(htonl(val)) {}
};
#pragma pack(pop)

static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint16_t s_port = 8443;

template<typename T>
std::span<const char> as_char_span(const T& obj) {
    return std::span<const char>(reinterpret_cast<const char*>(&obj), sizeof(T));
}

template<typename T>
std::span<char> as_writable_char_span(T& obj) {
    return std::span<char>(reinterpret_cast<char*>(&obj), sizeof(T));
}

int main() {

    TlsConnection tls_conn { s_port };
    tls_conn.listen();
    std::cout << "[ion] Listening on port " << s_port << "..." << std::endl;

    tls_conn.accept();
    std::cout << "Client connected." << std::endl;

    tls_conn.handshake("cert.pem", "key.pem");
    std::cout << "SSL handshake completed successfully." << std::endl;

    const std::string client_preface { "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n" };
    std::array<char, 1024> buffer{};

    ssize_t rx_len = tls_conn.read(buffer);
    if (rx_len < 0) {
        perror("read");
    } else if (rx_len >= static_cast<ssize_t>(client_preface.size())) {
        std::string_view received(buffer.data(), rx_len);
        if (received.starts_with(client_preface)) {
            std::cout << "Valid HTTP/2 preface received!" << std::endl;
        } else {
            std::cerr << "Invalid HTTP/2 preface" << std::endl;
        }
    } else {
        std::cerr << "Received data too short to be a valid preface" << std::endl;
    }

    // send SETTINGS frame
    std::vector<Http2Setting> settings = {
        {0x0003, 100},      // MAX_CONCURRENT_STREAMS
        {0x0004, 65535},    // INITIAL_WINDOW_SIZE
        {0x0005, 16384}     // MAX_FRAME_SIZE
    };

    Http2FrameHeader header;
    header.set_length(settings.size() * sizeof(Http2Setting));
    header.type = FRAME_TYPE_SETTINGS;
    header.flags = 0x00;
    header.set_stream_id(0);

    // Send header
    ssize_t sent = tls_conn.write(as_char_span(header));
    if (sent < 0) {
        perror("SSL_write header");
    }

    // Send settings payload
    sent = tls_conn.write(std::span{
        reinterpret_cast<const char*>(settings.data()),
        settings.size() * sizeof(Http2Setting)
    });
    if (sent < 0) {
        perror("SSL_write settings");
    } else {
        std::cout << "SETTINGS frame sent" << std::endl;
    }

    // Send header ack
    Http2FrameHeader ack_header;
    ack_header.set_length(0);
    ack_header.type = FRAME_TYPE_SETTINGS;
    ack_header.flags = 0x01;
    ack_header.set_stream_id(0);
    sent = tls_conn.write(as_char_span(ack_header));
    if (sent < 0) {
        perror("SSL_write header");
    }

    tls_conn.close();
    std::cout << "Connection closed." << std::endl;

    return 0;
}