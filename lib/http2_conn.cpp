#include "http2_conn.h"

#include <spdlog/spdlog.h>

#include <format>

#include "access_log.h"
#include "hpack/header_block_decoder.h"
#include "http2_frame_reader.h"
#include "http2_server.h"
#include "version.h"

namespace ion {

static constexpr uint8_t FRAME_TYPE_DATA = 0x00;
static constexpr uint8_t FRAME_TYPE_HEADERS = 0x01;
static constexpr uint8_t FRAME_TYPE_SETTINGS = 0x04;
static constexpr uint8_t FRAME_TYPE_GOAWAY = 0x07;
static constexpr uint8_t FRAME_TYPE_WINDOW_UPDATE = 0x08;

static constexpr uint8_t FLAG_END_HEADERS = 0x04;
static constexpr uint8_t FLAG_END_STREAM = 0x01;

static constexpr std::string_view CLIENT_PREFACE{"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"};

static constexpr size_t MAX_FRAME_SIZE = 16384;

static constexpr size_t READ_BUFFER_SIZE = 512 * 1024;

Http2Connection::Http2Connection(std::unique_ptr<Transport> transport, const std::string& client_ip,
                                 const Router& router)
    : transport_(std::move(transport)), client_ip_(client_ip), router_(router) {}

Http2WindowUpdate Http2Connection::process_window_update_payload(std::span<const uint8_t> payload) {
    return Http2WindowUpdate::parse(payload.subspan<0, Http2WindowUpdate::wire_size>());
}

void Http2Connection::write_frame_header(const Http2FrameHeader& header) {
    std::array<uint8_t, Http2FrameHeader::wire_size> header_bytes{};
    header.serialize(header_bytes);
    transport_->write(header_bytes);
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
        transport_->write(setting_bytes);
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
    transport_->write(headers_data);
}

void Http2Connection::write_data_response(uint32_t stream_id, const std::vector<uint8_t>& body) {
    size_t remaining_bytes = body.size();
    size_t start = 0;

    while (remaining_bytes != 0) {
        size_t chunk_size = std::min(remaining_bytes, MAX_FRAME_SIZE - 128);
        remaining_bytes -= chunk_size;

        const Http2FrameHeader frame_header{
            .length = static_cast<uint32_t>(chunk_size),
            .type = FRAME_TYPE_DATA,
            .flags = remaining_bytes == 0 ? FLAG_END_STREAM : static_cast<uint8_t>(0),
            .stream_id = stream_id};

        write_frame_header(frame_header);

        spdlog::trace("Writing data frame for stream {} with size {} & remaining {}", stream_id,
                      chunk_size, remaining_bytes);

        auto bytes_written_res = transport_->write(std::span(body.data() + start, chunk_size));
        if (!bytes_written_res) {
            spdlog::error("Failed to write data frame for stream {}: error: {}", stream_id,
                          static_cast<int>(bytes_written_res.error()));
            if (bytes_written_res.error() == TransportError::ProtocolError) {
                update_state(Http2ConnectionState::ProtocolError);
                return;
            }
        } else {
            if (*bytes_written_res != chunk_size) {
                spdlog::error("Failed to write data frame for stream {}: only wrote {} of {} bytes",
                              stream_id, *bytes_written_res, chunk_size);
            }
        }

        start += chunk_size;
    }
}

void Http2Connection::write_goaway(uint32_t last_stream_id, ErrorCode error_code) {
    const Http2GoAwayPayload payload{.last_stream_id = last_stream_id,
                                     .error_code = static_cast<uint32_t>(error_code)};

    const Http2FrameHeader header{.length = Http2GoAwayPayload::wire_size,
                                  .type = FRAME_TYPE_GOAWAY,
                                  .flags = 0x00,
                                  .stream_id = 0};

    write_frame_header(header);

    std::array<uint8_t, Http2GoAwayPayload::wire_size> payload_bytes{};
    payload.serialize(payload_bytes);
    transport_->write(payload_bytes);
}

void Http2Connection::write_settings() {
    const std::vector<Http2Setting> settings = {{0x0003, 100},    // MAX_CONCURRENT_STREAMS
                                                {0x0004, 65535},  // INITIAL_WINDOW_SIZE
                                                {0x0005, MAX_FRAME_SIZE}};
    write_settings(settings);
    spdlog::debug("SETTINGS frame sent");
}

void Http2Connection::populate_read_buffer() {
    std::vector<uint8_t> temp_buffer(READ_BUFFER_SIZE);
    const auto result = transport_->read(temp_buffer);

    if (!result) {
        switch (result.error()) {
            case TransportError::WantReadOrWrite:
                spdlog::trace("TLS want read/write");
                break;
            case TransportError::ConnectionClosed:
                spdlog::debug("TLS connection closed");
                update_state(Http2ConnectionState::ClientClosed);
                break;
            case TransportError::ProtocolError:
                spdlog::error("TLS protocol error");
                update_state(Http2ConnectionState::ProtocolError);
                break;
            case TransportError::WriteError:
                spdlog::error("TLS write error");
                break;
            case TransportError::OtherError:
                spdlog::error("TLS other error (presumed non fatal)");
                break;
        }
        return;
    }
    const auto bytes_read = result.value();
    if (bytes_read > 0) {
        buffer_.insert(buffer_.end(), temp_buffer.begin(), temp_buffer.begin() + bytes_read);
        spdlog::trace("read {} bytes, buffer size now {}", bytes_read, buffer_.size());
    }
}

void Http2Connection::discard_processed_buffer(size_t length) {
    buffer_.erase(buffer_.begin(), buffer_.begin() + length);
}

ReadPrefaceResult Http2Connection::read_preface() {
    if (buffer_.size() < CLIENT_PREFACE.length()) {
        spdlog::trace("not enough data for preface ({} < {})", buffer_.size(),
                      CLIENT_PREFACE.length());
        return ReadPrefaceResult::NotEnoughData;
    }
    std::span<const uint8_t, CLIENT_PREFACE.size()> preface_span{buffer_.data(),
                                                                 CLIENT_PREFACE.size()};
    const std::string_view received(reinterpret_cast<const char*>(preface_span.data()),
                                    preface_span.size());
    if (received != CLIENT_PREFACE) {
        spdlog::error("invalid HTTP/2 preface received");
        return ReadPrefaceResult::ProtocolError;
    }

    spdlog::info("valid HTTP/2 preface received");
    write_settings();
    update_state(Http2ConnectionState::AwaitingFrame);
    discard_processed_buffer(CLIENT_PREFACE.size());
    return ReadPrefaceResult::Success;
}

bool Http2Connection::try_read_frame() {
    if (buffer_.size() < Http2FrameHeader::wire_size) {
        spdlog::trace("not enough data for frame header ({} < {})", buffer_.size(),
                      Http2FrameHeader::wire_size);
        return false;
    }

    std::span<const uint8_t> buffer{buffer_};
    const auto header = Http2FrameHeader::parse(buffer.subspan<0, Http2FrameHeader::wire_size>());
    spdlog::debug("received frame header: type: {}, flag: {:#04x}, length: {}", header.type,
                  header.flags, header.length);

    const size_t total_frame_size = Http2FrameHeader::wire_size + header.length;
    if (buffer_.size() < total_frame_size) {
        spdlog::trace("not enough data for whole frame ({} < {})", buffer_.size(),
                      total_frame_size);
        return false;
    }

    Http2FrameReader frame{header, buffer.subspan(Http2FrameHeader::wire_size, header.length)};
    process_frame(frame);
    discard_processed_buffer(total_frame_size);
    return true;
}

void Http2Connection::process_frame(const Http2FrameReader& frame) {
    switch (frame.type()) {
        case FRAME_TYPE_SETTINGS: {
            spdlog::debug("received SETTINGS frame (stream={} flags={})", frame.stream_id(),
                          frame.flags());
            if (frame.has_flag(0x01)) {
                spdlog::debug("received SETTINGS ACK");
            } else {
                auto settings = frame.read_settings();
                if (!settings) {
                    update_state(Http2ConnectionState::ProtocolError);
                    return;
                }
                spdlog::debug("read {} settings, sending ACK", settings->size());
                write_settings_ack();
            }
            break;
        }
        case FRAME_TYPE_HEADERS: {
            spdlog::debug("received HEADERS frame for stream {}", frame.stream_id());
            spdlog::debug(" - end headers: {}, end stream: {}, length: {}", frame.is_end_headers(),
                          frame.is_end_stream(), frame.length());

            log_dynamic_tables();
            auto hdrs = decoder_.decode(frame.headers_block());
            if (!hdrs) {
                update_state(Http2ConnectionState::ProtocolError);
                return;
            }

            for (const auto& hdr : *hdrs) {
                spdlog::debug(" - request header: {}: {}", hdr.name, hdr.value);
            }

            auto resp = process_request(*hdrs);
            auto hdrs_bytes = encoder_.encode(resp.headers);
            log_dynamic_tables();

            auto ending_stream = resp.body.empty();
            write_headers_response(frame.stream_id(), hdrs_bytes,
                                   FLAG_END_HEADERS | (ending_stream ? FLAG_END_STREAM : 0));
            spdlog::info(std::format("{} status code sent w/headers", resp.status_code));

            if (!ending_stream) {
                spdlog::info("sending response body (length: {})", resp.body.size());
                write_data_response(frame.stream_id(), resp.body);
            }

            AccessLog::log_request(*hdrs, resp.status_code, resp.body.size(), client_ip_);
            break;
        }
        case FRAME_TYPE_WINDOW_UPDATE: {
            spdlog::debug("received WINDOW_UPDATE frame for stream {}", frame.stream_id());
            const auto [window_size_increment] = frame.read_window_update();
            spdlog::debug("window size increment = {}", window_size_increment);
            break;
        }
        case FRAME_TYPE_GOAWAY: {
            spdlog::debug("received GOAWAY frame (stream {})", frame.stream_id());
            update_state(Http2ConnectionState::ClientClosed);
            break;
        }
        default:
            spdlog::debug("received unknown frame type: {}", frame.type());
            break;
    }
}

constexpr std::string_view state_to_string(Http2ConnectionState state) {
    switch (state) {
        case Http2ConnectionState::AwaitingPreface:
            return "AwaitingPreface";
        case Http2ConnectionState::AwaitingFrame:
            return "AwaitingFrame";
        case Http2ConnectionState::ProtocolError:
            return "ProtocolError";
        case Http2ConnectionState::ClientClosed:
            return "ClientClosed";
    }
    return "Unknown";
}

Http2ProcessResult Http2Connection::process_state() {
    spdlog::trace("processing state: {}", state_to_string(state_));

    if (state_ == Http2ConnectionState::AwaitingPreface ||
        state_ == Http2ConnectionState::AwaitingFrame) {
        populate_read_buffer();  // updates connection state on error
    }

    switch (state_) {
        case Http2ConnectionState::AwaitingPreface: {
            switch (read_preface()) {
                case ReadPrefaceResult::Success:
                    return Http2ProcessResult::Complete;
                case ReadPrefaceResult::NotEnoughData:
                    return Http2ProcessResult::WantRead;
                case ReadPrefaceResult::ProtocolError:
                    return Http2ProcessResult::ProtocolError;
            }
        }
        case Http2ConnectionState::AwaitingFrame: {
            if (try_read_frame()) {
                spdlog::debug("frame processed, continuing...");
                return Http2ProcessResult::Complete;
            }
            return Http2ProcessResult::WantRead;
        }
        case Http2ConnectionState::ProtocolError: {
            return Http2ProcessResult::ProtocolError;
        }
        case Http2ConnectionState::ClientClosed: {
            return Http2ProcessResult::ClientClosed;
        }
        default: {
            throw std::runtime_error("invalid state");
        }
    }
}

void Http2Connection::close() {
    write_goaway(1, (state_ != Http2ConnectionState::ProtocolError) ? ErrorCode::no_error
                                                                    : ErrorCode::protocol_error);
    spdlog::debug("GOAWAY frame sent");
    transport_->graceful_shutdown();
}

void Http2Connection::update_state(Http2ConnectionState new_state) {
    if (new_state != state_) {
        spdlog::debug("connection state changed from {} to {}", state_to_string(state_),
                      state_to_string(new_state));
        state_ = new_state;
    }
}

void Http2Connection::log_dynamic_tables() {
    spdlog::debug("decoder dynamic table:");
    decoder_dynamic_table_.log_contents();
    spdlog::debug("encoder dynamic table:");
    encoder_dynamic_table_.log_contents();
}

static std::optional<std::string> get_header(const std::vector<HttpHeader>& headers,
                                             const std::string& name) {
    const auto it = std::find_if(headers.begin(), headers.end(),
                                 [name](const HttpHeader& hdr) { return hdr.name == name; });
    if (it == headers.end()) {
        return std::nullopt;
    }
    return it->value;
}

HttpResponse Http2Connection::process_request(const std::vector<HttpHeader>& headers) {
    auto path = get_header(headers, ":path");
    auto method = get_header(headers, ":method");

    if (!path || !method) {
        spdlog::error("invalid request: missing path or method");
        return HttpResponse{.status_code = 400};
    }

    auto handler = router_.get_handler(path.value(), method.value());
    HttpRequest req{.method = *method, .path = *path, .headers = headers};
    auto resp = handler(req);
    resp.headers.insert(resp.headers.begin(),
                        HttpHeader{":status", std::to_string(resp.status_code)});
    if (!resp.body.empty()) {
        resp.headers.push_back({"content-length", std::to_string(resp.body.size())});
    }
    resp.headers.push_back({"server", std::string{SERVER_HEADER}});
    resp.headers.push_back({"x-powered-by", std::string{SERVER_HEADER}});
    return resp;
}

}  // namespace ion
