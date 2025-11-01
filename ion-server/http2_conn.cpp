#include "http2_conn.h"

#include <format>

#include "spdlog/spdlog.h"

static constexpr uint8_t FRAME_TYPE_HEADERS = 0x01;
static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint8_t FRAME_TYPE_GOAWAY = 0x07;
static constexpr uint8_t FRAME_TYPE_WINDOW_UPDATE = 0x08;

static constexpr uint8_t FLAG_END_HEADERS = 0x04;
static constexpr uint8_t FLAG_END_STREAM = 0x01;

static constexpr std::string_view CLIENT_PREFACE{"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"};

static constexpr size_t PREFACE_BUFFER_SIZE = CLIENT_PREFACE.size();
static constexpr size_t READ_BUFFER_SIZE = 1024 * 1024;

Http2Connection::Http2Connection(const TlsConnection& conn) : tls_conn_(conn) {}

bool Http2Connection::try_read_preface() {
    std::array<uint8_t, PREFACE_BUFFER_SIZE> temp_buffer{};
    const auto bytes_read = tls_conn_.read(temp_buffer);

    if (bytes_read > 0) {
        // Append new data to our buffer
        buffer_.insert(buffer_.end(), temp_buffer.begin(), temp_buffer.begin() + bytes_read);
    }

    // Check if we have enough data for at least a frame header
    if (buffer_.size() < CLIENT_PREFACE.length()) {
        spdlog::debug("Not enough data");
        return false;  // Not enough data yet
    }

    std::span<const uint8_t, CLIENT_PREFACE.size()> preface_span{buffer_.data(),
                                                                 CLIENT_PREFACE.size()};
    const std::string_view received(reinterpret_cast<const char*>(preface_span.data()),
                                    preface_span.size());
    if (received != CLIENT_PREFACE) {
        throw std::runtime_error("invalid HTTP/2 preface");
    }

    spdlog::debug("received HTTP/2 preface");

    buffer_.erase(buffer_.begin(), buffer_.begin() + CLIENT_PREFACE.size());
    return true;
}

void Http2Connection::process_settings_payload(std::span<const uint8_t> payload) {
    std::vector<Http2Setting> settings;
    const auto num_settings = payload.size() / Http2Setting::wire_size;
    settings.reserve(num_settings);

    for (size_t i = 0; i < num_settings; ++i) {
        std::span<const uint8_t, Http2Setting::wire_size> entry{
            payload.data() + i * Http2Setting::wire_size, Http2Setting::wire_size};
        settings.push_back(Http2Setting::parse(entry));
    }
    spdlog::debug("Received {} SETTINGS. Sending ACK", settings.size());
    write_settings_ack();
}

Http2WindowUpdate Http2Connection::process_window_update_payload(std::span<const uint8_t> payload) {
    return Http2WindowUpdate::parse(payload.subspan<0, Http2WindowUpdate::wire_size>());
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

void Http2Connection::write_settings() {
    const std::vector<Http2Setting> settings = {
        {0x0003, 100},    // MAX_CONCURRENT_STREAMS
        {0x0004, 65535},  // INITIAL_WINDOW_SIZE
        {0x0005, 16384}   // MAX_FRAME_SIZE
    };
    write_settings(settings);
    spdlog::debug("SETTINGS frame sent");
}

bool Http2Connection::try_read_frame() {
    std::array<uint8_t, READ_BUFFER_SIZE> temp_buffer{};
    const auto bytes_read = tls_conn_.read(temp_buffer);

    if (bytes_read > 0) {
        // Append new data to our buffer
        buffer_.insert(buffer_.end(), temp_buffer.begin(), temp_buffer.begin() + bytes_read);
    }

    // Check if we have enough data for at least a frame header
    if (buffer_.size() < Http2FrameHeader::wire_size) {
        spdlog::debug("Not enough data");
        return false;  // Not enough data yet
    }

    // Parse the frame header from the beginning of the buffer
    std::span<const uint8_t, Http2FrameHeader::wire_size> header_span{buffer_.data(),
                                                                      Http2FrameHeader::wire_size};
    const auto header = Http2FrameHeader::parse(header_span);
    spdlog::debug("received frame header: type: {}, flag: {:#04x}, length: {}", header.type,
                  header.flags, header.length);

    // Check if we have the complete frame (header + payload)
    const size_t total_frame_size = Http2FrameHeader::wire_size + header.length;
    if (buffer_.size() < total_frame_size) {
        return false;  // Not enough data for complete frame yet
    }

    spdlog::debug("received complete frame");
    process_frame(header, std::span<const uint8_t>{buffer_.data() + Http2FrameHeader::wire_size,
                                                   header.length});
    // Remove the processed frame from the buffer
    buffer_.erase(buffer_.begin(), buffer_.begin() + total_frame_size);

    return true;
}

void Http2Connection::process_frame(const Http2FrameHeader& header,
                                    std::span<const uint8_t> payload) {
    spdlog::debug("Processing frame: type={}, stream_id={}", header.type, header.stream_id);

    switch (header.type) {
        case FRAME_TYPE_SETTINGS: {
            if (header.flags & 0x01) {
                spdlog::debug("Received SETTINGS ACK");
            } else {
                if (header.length % Http2Setting::wire_size != 0) {
                    spdlog::error("Invalid SETTINGS frame: data size not a multiple of {}",
                                  Http2Setting::wire_size);
                    return;
                }

                process_settings_payload(payload);
            }
            break;
        }
        case FRAME_TYPE_HEADERS: {
            spdlog::debug("Received HEADERS frame for stream {}", header.stream_id);
            const bool end_headers_set = (header.flags & FLAG_END_HEADERS) != 0;
            const bool end_stream_set = (header.flags & FLAG_END_STREAM) != 0;
            spdlog::debug(" - stream ID: {}, end headers: {}, end stream: {}, length: {}",
                          header.stream_id, end_headers_set, end_stream_set, header.length);

            // Send 200 response: HPACK index 8 = ":status: 200"
            std::array<uint8_t, 1> headers_data = {0x88};  // 0x80 | 8 = indexed header
            write_headers_response(1, headers_data, FLAG_END_HEADERS | FLAG_END_STREAM);
            spdlog::info("200 response sent");
            break;
        }
        case FRAME_TYPE_WINDOW_UPDATE: {
            spdlog::debug("Received WINDOW_UPDATE frame for stream {}", header.stream_id);
            process_window_update_payload(payload);
            break;
        }
        default:
            spdlog::debug("Received unknown frame type: {}", header.type);
            break;
    }
}
