#include <openssl/ssl.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include "http2_frames.h"
#include "tls_conn.h"

static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint16_t s_port = 8443;

template<typename T>
std::span<const char> as_char_span(const T& obj) {
    return std::span(reinterpret_cast<const char*>(&obj), sizeof(T));
}

template<typename T>
std::span<char> as_writable_char_span(T& obj) {
    return std::span(reinterpret_cast<char*>(&obj), sizeof(T));
}

void read_preface(TlsConnection& conn) {
    constexpr std::string_view client_preface { "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n" };
    std::array<char, client_preface.length()> buffer{};

    const auto bytes_read = conn.read(buffer);
    std::string_view received(buffer.data(), bytes_read);
    if (bytes_read == client_preface.length()) {
        if (received.starts_with(client_preface)) {
        } else {
            throw std::runtime_error("Invalid HTTP/2 preface");
        }
    } else {
        throw std::runtime_error("Received data too short to be a valid preface");
    }
}

void run_server() {
    TlsConnection tls_conn { s_port };
    tls_conn.listen();
    std::cout << "[ion] Listening on port " << s_port << "..." << std::endl;

    tls_conn.accept();
    std::cout << "Client connected." << std::endl;

    tls_conn.handshake("cert.pem", "key.pem");
    std::cout << "SSL handshake completed successfully." << std::endl;

    read_preface(tls_conn);
    std::cout << "Valid HTTP/2 preface received!" << std::endl;

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