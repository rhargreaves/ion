#pragma once
#include <expected>
#include <span>
#include <vector>

#include "frame_error.h"
#include "http2_frames.h"

namespace ion {

class Http2FrameReader {
   public:
    explicit Http2FrameReader(const Http2FrameHeader& header, std::span<const uint8_t> payload)
        : header_(header), payload_(payload) {}

    uint8_t type() const {
        return header_.type;
    }

    uint8_t flags() const {
        return header_.flags;
    }

    uint32_t stream_id() const {
        return header_.stream_id;
    }

    uint32_t length() const {
        return header_.length;
    }

    std::span<const uint8_t> payload() const {
        return payload_;
    }

    bool has_flag(uint8_t flag) const {
        return (header_.flags & flag) != 0;
    }

    bool is_end_stream() const {
        return has_flag(0x01);
    }

    bool is_end_headers() const {
        return has_flag(0x04);
    }

    bool priority() const {
        return has_flag(0x20);
    }

    bool padded() const {
        return has_flag(0x08);
    }

    // Type-specific payload readers
    std::expected<std::vector<Http2Setting>, FrameError> read_settings() const;
    Http2WindowUpdate read_window_update() const;
    Http2GoAwayPayload read_goaway() const;

    std::span<const uint8_t> read_headers_block() const {
        if (priority()) {
            return payload_.subspan(5);  // skip exclusive (1), stream dependency (31) & weight (8)
        }
        return payload_;
    }

    std::span<const uint8_t> read_data() const {
        return payload_;
    }

   private:
    Http2FrameHeader header_;
    std::span<const uint8_t> payload_;
};

}  // namespace ion
