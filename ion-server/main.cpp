#include <openssl/ssl.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include "http2_frames.h"
#include "tls_conn.h"

static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint16_t SERVER_PORT = 8443;

template<typename T>
std::span<const char> as_char_span(const T& obj) {
    return {reinterpret_cast<const char*>(&obj), sizeof(T)};
}

template<typename T>
std::span<char> as_writable_char_span(T& obj) {
    return {reinterpret_cast<char*>(&obj), sizeof(T)};
}

void read_preface(const TlsConnection& conn) {
    constexpr std::string_view client_preface { "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n" };
    std::array<char, client_preface.length()> buffer{};

    const auto bytes_read = conn.read(buffer);
    const std::string_view received(buffer.data(), bytes_read);
    if (bytes_read == client_preface.length()) {
        if (!received.starts_with(client_preface)) {
            throw std::runtime_error("Invalid HTTP/2 preface");
        }
    } else {
        throw std::runtime_error("Received data too short to be a valid preface");
    }
}

void write_settings_header(const TlsConnection& conn, const std::vector<Http2Setting>& settings) {
    Http2FrameHeader header{};
    header.set_length(settings.size() * sizeof(Http2Setting));
    header.type = FRAME_TYPE_SETTINGS;
    header.flags = 0x00;
    header.set_stream_id(0);
    conn.write(as_char_span(header));
    conn.write(std::span{
        reinterpret_cast<const char*>(settings.data()),
        settings.size() * sizeof(Http2Setting)
    });
}

void write_settings_ack(const TlsConnection& conn) {
    Http2FrameHeader header{};
    header.set_length(0);
    header.type = FRAME_TYPE_SETTINGS;
    header.flags = 0x01;
    header.set_stream_id(0);
    conn.write(as_char_span(header));
}

void read_and_ignore_settings_header(const TlsConnection& conn) {
    Http2FrameHeader header{};
    auto buffer = as_writable_char_span(header);
    auto bytes_read = conn.read(buffer);
    if (bytes_read != static_cast<ssize_t>(buffer.size())) {
        throw std::runtime_error("Failed to read settings header (not enough bytes)");
    }
    auto data = std::vector<char>(header.get_length());
    bytes_read = conn.read(data);
    if (bytes_read != static_cast<ssize_t>(data.size())) {
        throw std::runtime_error("Failed to read settings body header (not enough bytes)");
    }
}

void read_settings_ack(const TlsConnection& conn) {
    Http2FrameHeader header{};
    auto buffer = as_writable_char_span(header);
    auto bytes_read = conn.read(buffer);
    if (bytes_read != static_cast<ssize_t>(buffer.size())) {
        throw std::runtime_error("Failed to read settings header (not enough bytes)");
    }
    if (header.type != FRAME_TYPE_SETTINGS) {
        throw std::runtime_error("Received frame was not a SETTINGS frame. Got type " +
            std::to_string(header.type));
    }
    if (header.flags != 0x01) {
        throw std::runtime_error("Received SETTINGS frame was not an ACK");
    }
    if (header.stream_id != 0) {
        throw std::runtime_error("Received SETTINGS frame had a non-zero stream ID");
    }
}

void run_server() {
    TlsConnection tls_conn { SERVER_PORT };
    tls_conn.listen();
    std::cout << "[ion] Listening on port " << SERVER_PORT << "..." << std::endl;

    tls_conn.accept();
    std::cout << "Client connected." << std::endl;

    tls_conn.handshake("cert.pem", "key.pem");
    std::cout << "SSL handshake completed successfully." << std::endl;

    read_preface(tls_conn);
    std::cout << "Valid HTTP/2 preface received!" << std::endl;

    read_and_ignore_settings_header(tls_conn);
    std::cout << "SETTINGS frame received" << std::endl;

   // write_settings_ack(tls_conn);
   // std::cout << "SETTINGS ACK frame sent" << std::endl;

    const std::vector<Http2Setting> settings = {
        {0x0003, 100},      // MAX_CONCURRENT_STREAMS
        {0x0004, 65535},    // INITIAL_WINDOW_SIZE
        {0x0005, 16384}     // MAX_FRAME_SIZE
    };
    write_settings_header(tls_conn, settings);
    std::cout << "SETTINGS frame sent" << std::endl;

    read_settings_ack(tls_conn);
    std::cout << "SETTINGS ACK frame received" << std::endl;

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
