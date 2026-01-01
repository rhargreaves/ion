#pragma once
#include <chrono>
#include <span>
#include <vector>

#include "hpack/header_block_decoder.h"
#include "hpack/header_block_encoder.h"
#include "http2_frame_reader.h"
#include "http2_frames.h"
#include "router.h"
#include "transports/transport.h"

namespace ion {

enum class Http2ProcessResult {
    WantRead,
    WantWrite,
    DiscardConnection,
};

enum class Http2ConnectionState {
    AwaitingHandshake,
    AwaitingPreface,
    AwaitingFrame,
    Closing,
    ProtocolError,
    ClientClosed
};

enum class ReadPrefaceResult { Success, NotEnoughData, ProtocolError };

class Http2Connection {
   public:
    explicit Http2Connection(std::unique_ptr<Transport> transport, const std::string& client_ip,
                             const Router& router);
    Http2Connection(const Http2Connection&) = delete;
    Http2Connection& operator=(const Http2Connection&) = delete;
    Http2Connection(Http2Connection&&) = delete;
    Http2Connection& operator=(Http2Connection&&) = delete;

    Http2ProcessResult process();
    void close();
    [[nodiscard]] bool has_timed_out() const;

   private:
    std::unique_ptr<Transport> transport_;
    std::string client_ip_;
    const Router& router_;
    std::vector<uint8_t> read_buffer_;
    std::vector<uint8_t> write_buffer_;
    Http2ConnectionState state_ = Http2ConnectionState::AwaitingHandshake;
    DynamicTable decoder_dynamic_table_{};
    DynamicTable encoder_dynamic_table_{};
    HeaderBlockDecoder decoder_{decoder_dynamic_table_};
    HeaderBlockEncoder encoder_{encoder_dynamic_table_};
    std::chrono::steady_clock::time_point last_activity_{std::chrono::steady_clock::now()};

    Http2ProcessResult internal_process();
    ReadPrefaceResult read_preface();
    Http2WindowUpdate process_window_update_payload(std::span<const uint8_t> payload);
    bool try_read_frame();
    void populate_read_buffer();
    void discard_processed_buffer(size_t length);
    void write_frame_header(const Http2FrameHeader& header);
    void write_settings(const std::vector<Http2Setting>& settings);
    void write_settings_ack();
    void write_headers_response(uint32_t stream_id, std::span<const uint8_t> headers_data,
                                uint8_t flags);
    void write_data_response(uint32_t uint32, const std::vector<uint8_t>& body);
    void write_goaway(uint32_t last_stream_id, ErrorCode error_code);
    void write_settings();
    void process_frame(const Http2FrameReader& frame);
    void update_state(Http2ConnectionState new_state);
    void log_dynamic_tables();
    HttpResponse process_request(const std::vector<HttpHeader>& req_hdrs);
    void enqueue_write(std::span<const uint8_t> data);
    void flush_write_buffer();
    void update_last_activity();
};

}  // namespace ion
