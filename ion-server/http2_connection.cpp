#include "http2_connection.h"

#include <format>
#include <iostream>

static constexpr uint8_t FRAME_TYPE_HEADERS = 0x01;
static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint8_t FRAME_TYPE_GOAWAY = 0x07;

Http2Connection::Http2Connection(const TlsConnection& conn) : tls_conn(conn) {}

void Http2Connection::read_exact(std::span<uint8_t> buffer) {
    size_t total_read = 0;
    while (total_read < buffer.size()) {
        const auto bytes_read = tls_conn.read(buffer.subspan(total_read));
        if (bytes_read == 0) {
            throw std::runtime_error("Connection closed unexpectedly");
        }
        total_read += bytes_read;
    }
}

void Http2Connection::read_preface() {
    constexpr std::string_view client_preface{"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"};
    std::array<uint8_t, client_preface.length()> buffer{};

    read_exact(buffer);
    const std::string_view received(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    if (received != client_preface) {
        throw std::runtime_error("Invalid HTTP/2 preface");
    }
}

Http2FrameHeader Http2Connection::read_frame_header() {
    std::array<uint8_t, 9> header_bytes{};
    read_exact(header_bytes);
    return Http2FrameHeader::parse(header_bytes);
}

std::vector<uint8_t> Http2Connection::read_payload(uint32_t length) {
    std::vector<uint8_t> data(length);
    read_exact(data);
    return data;
}

std::vector<Http2Setting> Http2Connection::read_settings_payload(uint32_t length) {
    constexpr size_t kSettingSize = 6;
    if (length % kSettingSize != 0) {
        throw std::runtime_error(
            std::format("Invalid SETTINGS frame: data size not a multiple of {}", kSettingSize));
    }

    auto data = read_payload(length);
    std::vector<Http2Setting> settings;
    const auto num_settings = data.size() / kSettingSize;
    settings.reserve(num_settings);

    for (size_t i = 0; i < num_settings; ++i) {
        std::span<const uint8_t, 6> entry{data.data() + i * kSettingSize, kSettingSize};
        settings.push_back(Http2Setting::parse(entry));
    }
    return settings;
}

void Http2Connection::write_frame_header(const Http2FrameHeader& header) {
    std::array<uint8_t, 9> header_bytes{};
    header.serialize(header_bytes);
    tls_conn.write(header_bytes);
}

void Http2Connection::write_settings(const std::vector<Http2Setting>& settings) {
    const Http2FrameHeader header{.length = static_cast<uint32_t>(settings.size() * 6),
                                  .type = FRAME_TYPE_SETTINGS,
                                  .flags = 0x00,
                                  .stream_id = 0};

    write_frame_header(header);

    for (const auto& setting : settings) {
        std::array<uint8_t, 6> setting_bytes{};
        setting.serialize(setting_bytes);
        tls_conn.write(setting_bytes);
    }
}

void Http2Connection::write_settings_ack() {
    const Http2FrameHeader header{
        .length = 0, .type = FRAME_TYPE_SETTINGS, .flags = 0x01, .stream_id = 0};
    write_frame_header(header);
}

void Http2Connection::write_headers_response(uint32_t stream_id,
                                             std::span<const uint8_t> headers_data, uint8_t flags) {
    const Http2FrameHeader header{.length = static_cast<uint32_t>(headers_data.size()),
                                  .type = FRAME_TYPE_HEADERS,
                                  .flags = flags,
                                  .stream_id = stream_id};

    write_frame_header(header);
    tls_conn.write(headers_data);
}

void Http2Connection::write_goaway(uint32_t last_stream_id, uint32_t error_code) {
    const Http2GoAwayPayload payload{.last_stream_id = last_stream_id, .error_code = error_code};

    const Http2FrameHeader header{
        .length = 8, .type = FRAME_TYPE_GOAWAY, .flags = 0x00, .stream_id = 0};

    write_frame_header(header);

    std::array<uint8_t, 8> payload_bytes{};
    payload.serialize(payload_bytes);
    tls_conn.write(payload_bytes);
}
