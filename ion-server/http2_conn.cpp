#include "http2_conn.h"

#include <format>
#include <iostream>
#include <thread>

#include "http2_except.h"
#include "spdlog/spdlog.h"

static constexpr uint8_t FRAME_TYPE_HEADERS = 0x01;
static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint8_t FRAME_TYPE_GOAWAY = 0x07;

Http2Connection::Http2Connection(const TlsConnection& conn) : tls_conn_(conn) {}

void Http2Connection::read_exact(std::span<uint8_t> buffer) {
    size_t total_read = 0;
    constexpr auto timeout = std::chrono::milliseconds(500);
    auto start_time = std::chrono::steady_clock::now();

    while (total_read < buffer.size()) {
        if (std::chrono::steady_clock::now() - start_time > timeout) {
            throw Http2TimeoutException();
        }

        const auto bytes_read = tls_conn_.read(buffer.subspan(total_read));
        if (bytes_read > 0) {
            total_read += bytes_read;
        } else if (bytes_read == 0) {
            // No data available - wait briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            throw std::runtime_error("Failed to read from TLS connection");
        }
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
    std::array<uint8_t, Http2FrameHeader::wire_size> header_bytes{};
    read_exact(header_bytes);
    return Http2FrameHeader::parse(header_bytes);
}

std::vector<uint8_t> Http2Connection::read_payload(uint32_t length) {
    std::vector<uint8_t> data(length);
    read_exact(data);
    return data;
}

std::vector<Http2Setting> Http2Connection::read_settings_payload(uint32_t length) {
    if (length % Http2Setting::wire_size != 0) {
        throw std::runtime_error(std::format(
            "Invalid SETTINGS frame: data size not a multiple of {}", Http2Setting::wire_size));
    }

    auto data = read_payload(length);
    std::vector<Http2Setting> settings;
    const auto num_settings = data.size() / Http2Setting::wire_size;
    settings.reserve(num_settings);

    for (size_t i = 0; i < num_settings; ++i) {
        std::span<const uint8_t, Http2Setting::wire_size> entry{
            data.data() + i * Http2Setting::wire_size, Http2Setting::wire_size};
        settings.push_back(Http2Setting::parse(entry));
    }
    return settings;
}

Http2WindowUpdate Http2Connection::read_window_update(uint32_t length) {
    if (length != Http2WindowUpdate::wire_size) {
        throw std::runtime_error(std::format("Invalid WINDOW_UPDATE frame: data size not {}",
                                             Http2WindowUpdate::wire_size));
    }

    auto data = read_payload(length);
    return Http2WindowUpdate::parse(std::span<const uint8_t, Http2WindowUpdate::wire_size>{
        data.data(), Http2WindowUpdate::wire_size});
}

void Http2Connection::write_frame_header(const Http2FrameHeader& header) {
    std::array<uint8_t, Http2FrameHeader::wire_size> header_bytes{};
    header.serialize(header_bytes);
    tls_conn_.write(header_bytes);
}

void Http2Connection::write_settings(const std::vector<Http2Setting>& settings) {
    const Http2FrameHeader header{
        .length = static_cast<uint32_t>(settings.size() * Http2Setting::wire_size),
        .type = FRAME_TYPE_SETTINGS,
        .flags = 0x00,
        .stream_id = 0};

    write_frame_header(header);

    for (const auto& setting : settings) {
        std::array<uint8_t, Http2Setting::wire_size> setting_bytes{};
        setting.serialize(setting_bytes);
        tls_conn_.write(setting_bytes);
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
    tls_conn_.write(headers_data);
}

void Http2Connection::write_goaway(uint32_t last_stream_id, uint32_t error_code) {
    const Http2GoAwayPayload payload{.last_stream_id = last_stream_id, .error_code = error_code};

    const Http2FrameHeader header{.length = Http2GoAwayPayload::wire_size,
                                  .type = FRAME_TYPE_GOAWAY,
                                  .flags = 0x00,
                                  .stream_id = 0};

    write_frame_header(header);

    std::array<uint8_t, Http2GoAwayPayload::wire_size> payload_bytes{};
    payload.serialize(payload_bytes);
    tls_conn_.write(payload_bytes);
}

bool Http2Connection::wait_for_client_disconnect() {
    constexpr int shutdown_timeout_ms = 1000;
    constexpr int poll_interval_ms = 100;
    int elapsed_ms = 0;

    while (elapsed_ms < shutdown_timeout_ms) {
        try {
            if (tls_conn_.has_data()) {
                // discard any remaining frames
                const auto header = read_frame_header();
                auto data = read_payload(header.length);
                spdlog::debug("received frame type {} during shutdown", +header.type);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval_ms));
                elapsed_ms += poll_interval_ms;
            }
        } catch (const std::exception& e) {
            spdlog::debug("client disconnected: {}", e.what());
            return true;
        }
    }
    spdlog::warn("shutdown timeout reached, forcing connection close");
    return false;
}
