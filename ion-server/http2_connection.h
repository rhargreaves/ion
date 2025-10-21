#pragma once

#include <span>
#include <vector>

#include "http2_frames.h"
#include "tls_conn.h"

class Http2Connection {
   public:
    explicit Http2Connection(const TlsConnection& tls_conn);

    void read_preface();
    Http2FrameHeader read_frame_header();
    std::vector<uint8_t> read_payload(uint32_t length);
    std::vector<Http2Setting> read_settings_payload(uint32_t length);
    Http2WindowUpdate read_window_update(uint32_t length);

    void write_frame_header(const Http2FrameHeader& header);
    void write_settings(const std::vector<Http2Setting>& settings);
    void write_settings_ack();
    void write_headers_response(uint32_t stream_id, std::span<const uint8_t> headers_data,
                                uint8_t flags);
    void write_goaway(uint32_t last_stream_id, uint32_t error_code = 0);

   private:
    const TlsConnection& tls_conn;

    void read_exact(std::span<uint8_t> buffer);
};
