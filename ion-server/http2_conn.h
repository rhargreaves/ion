#pragma once

#include <span>
#include <vector>

#include "http2_frames.h"
#include "tls_conn.h"

enum class Http2ProcessResult { AwaitingData, ProcessedData, ClosingCleanly };
enum class Http2ConnectionState { Preface, Frames, GoAway, ProtocolError };

class Http2Connection {
   public:
    explicit Http2Connection(const TlsConnection& tls_conn);
    Http2ProcessResult process_state();
    void close();

   private:
    const TlsConnection& tls_conn_;
    std::vector<uint8_t> buffer_;
    Http2ConnectionState state_ = Http2ConnectionState::Preface;

    bool try_read_preface();
    Http2WindowUpdate process_window_update_payload(std::span<const uint8_t> payload);
    bool try_read_frame();

    void write_frame_header(const Http2FrameHeader& header);
    void write_settings(const std::vector<Http2Setting>& settings);
    void write_settings_ack();
    void write_headers_response(uint32_t stream_id, std::span<const uint8_t> headers_data,
                                uint8_t flags);
    void write_goaway(uint32_t last_stream_id, uint32_t error_code = 0);
    void write_settings();
    void process_settings_payload(std::span<const uint8_t> payload);
    void process_frame(const Http2FrameHeader& header, std::span<const uint8_t> payload);
};
