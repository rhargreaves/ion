#include <sys/socket.h>

#include <iostream>
#include <vector>

#include "http2_frames.h"
#include "tls_conn.h"

static constexpr uint8_t FRAME_TYPE_HEADERS = 0x01;
static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint8_t FRAME_TYPE_GOAWAY = 0x07;

static constexpr uint8_t FLAG_END_HEADERS = 0x04;
static constexpr uint8_t FLAG_END_STREAM = 0x01;

static constexpr uint16_t SERVER_PORT = 8443;

template <typename T>
std::span<const char> as_char_span(const T& obj) {
    return {reinterpret_cast<const char*>(&obj), sizeof(T)};
}

template <typename T>
std::span<char> as_writable_char_span(T& obj) {
    return {reinterpret_cast<char*>(&obj), sizeof(T)};
}

void read_exact(const TlsConnection& conn, std::span<char> buffer) {
    size_t total_read = 0;
    while (total_read < buffer.size()) {
        const auto bytes_read = conn.read(buffer.subspan(total_read));
        if (bytes_read == 0) {
            throw std::runtime_error("Connection closed unexpectedly");
        }
        total_read += bytes_read;
    }
}

void read_preface(const TlsConnection& conn) {
    constexpr std::string_view client_preface{"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"};
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
    Http2FrameHeader header{.length = static_cast<uint32_t>(settings.size() * sizeof(Http2Setting)),
                            .type = FRAME_TYPE_SETTINGS,
                            .flags = 0x00,
                            .stream_id = 0};

    std::array<uint8_t, 9> header_bytes{};
    header.serialize(header_bytes);
    conn.write(std::span{reinterpret_cast<const char*>(header_bytes.data()), 9});
    conn.write(std::span{reinterpret_cast<const char*>(settings.data()),
                         settings.size() * sizeof(Http2Setting)});
}

void write_settings_ack(const TlsConnection& conn) {
    Http2FrameHeader header{
        .length = 0, .type = FRAME_TYPE_SETTINGS, .flags = 0x01, .stream_id = 0};
    std::array<uint8_t, 9> header_bytes{};
    header.serialize(header_bytes);
    conn.write(std::span{reinterpret_cast<const char*>(header_bytes.data()), 9});
}

std::vector<Http2Setting> read_settings(const std::span<char> data) {
    if (data.size() % sizeof(Http2Setting) != 0) {
        throw std::runtime_error(std::format(
            "Invalid SETTINGS frame: data size not a multiple of {}", sizeof(Http2Setting)));
    }

    std::vector<Http2Setting> settings;
    const auto num_settings = data.size() / sizeof(Http2Setting);
    settings.reserve(num_settings);

    auto* settings_ptr = reinterpret_cast<const Http2Setting*>(data.data());
    for (size_t i = 0; i < num_settings; ++i) {
        settings.push_back(settings_ptr[i]);
    }
    return settings;
}

void read_frame(const TlsConnection& conn) {
    constexpr size_t frame_header_length = 9;
    std::array<uint8_t, frame_header_length> header_bytes{};
    read_exact(conn, std::span{reinterpret_cast<char*>(header_bytes.data()), frame_header_length});

    const auto header = Http2FrameHeader::parse(header_bytes);
    switch (header.type) {
        case FRAME_TYPE_SETTINGS: {
            std::cout << "SETTINGS frame received" << std::endl;

            auto data = std::vector<char>(header.length);
            read_exact(conn, data);

            auto settings = read_settings(data);
            std::cout << "Received " << settings.size() << " settings" << std::endl;
            break;
        }
        default: {
            std::cout << "Received frame of type " << +header.type << std::endl;

            auto data = std::vector<char>(header.length);
            read_exact(conn, data);
            break;
        }
    }
}

void send_200_response(const TlsConnection& conn, uint32_t stream_id) {
    // HPACK: index 8 = ":status: 200"
    std::array<uint8_t, 1> headers_data = {0x88};  // 0x80 | 8 = indexed header
    std::array<uint8_t, 9> header_bytes{};

    Http2FrameHeader header{.length = static_cast<uint32_t>(headers_data.size()),
                            .type = FRAME_TYPE_HEADERS,
                            .flags = FLAG_END_HEADERS | FLAG_END_STREAM,
                            .stream_id = stream_id};

    header.serialize(header_bytes);
    conn.write(std::span{reinterpret_cast<const char*>(header_bytes.data()), 9});
    conn.write(std::span{reinterpret_cast<const char*>(headers_data.data()), headers_data.size()});
}

void send_goaway(const TlsConnection& conn, uint32_t last_stream_id) {
    Http2GoAwayPayload payload{last_stream_id, 0};
    Http2FrameHeader header{.length = static_cast<uint32_t>(sizeof(payload)),
                            .type = FRAME_TYPE_GOAWAY,
                            .flags = 0x00,
                            .stream_id = 0};

    std::array<uint8_t, 9> header_bytes{};
    header.serialize(header_bytes);
    conn.write(std::span{reinterpret_cast<const char*>(header_bytes.data()), 9});
    conn.write(as_char_span(payload));
}

void run_server() {
    TlsConnection tls_conn{SERVER_PORT};
    tls_conn.listen();
    std::cout << "[ion] Listening on port " << SERVER_PORT << "..." << std::endl;

    tls_conn.accept();
    std::cout << "Client connected." << std::endl;

    tls_conn.handshake("cert.pem", "key.pem");
    std::cout << "SSL handshake completed successfully." << std::endl;

    read_preface(tls_conn);
    std::cout << "Valid HTTP/2 preface received!" << std::endl;

    read_frame(tls_conn);

    const std::vector<Http2Setting> settings = {
        {0x0003, 100},    // MAX_CONCURRENT_STREAMS
        {0x0004, 65535},  // INITIAL_WINDOW_SIZE
        {0x0005, 16384}   // MAX_FRAME_SIZE
    };
    write_settings_header(tls_conn, settings);
    std::cout << "SETTINGS frame sent" << std::endl;

    read_frame(tls_conn);
    // read_settings_ack(tls_conn);
    // std::cout << "SETTINGS ACK frame received" << std::endl;

    send_200_response(tls_conn, 1);
    std::cout << "200 response sent" << std::endl;

    send_goaway(tls_conn, 1);
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
