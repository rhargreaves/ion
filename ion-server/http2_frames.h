#pragma once
#include <arpa/inet.h>

#pragma pack(push, 1)
struct Http2FrameHeader {
    uint8_t length[3];  // 24-bit, big-endian
    uint8_t type;
    uint8_t flags;
    uint32_t reserved_and_stream_id;  // Reserved (1 bit) + Stream ID (31 bits), big-endian

    void set_length(uint32_t len) {
        length[0] = (len >> 16) & 0xFF;
        length[1] = (len >> 8) & 0xFF;
        length[2] = len & 0xFF;
    }

    int get_length() const { return length[0] << 16 | length[1] << 8 | length[2]; }

    void set_stream_id(uint32_t id) { reserved_and_stream_id = htonl(id & 0x7FFFFFFF); }

    uint32_t get_stream_id() const { return ntohl(reserved_and_stream_id) & 0x7FFFFFFF; }
};

struct Http2WindowUpdate {
    uint8_t reserved : 1;
    uint32_t window_size_increment : 31;
};

struct Http2Setting {
    uint16_t identifier;
    uint32_t value;

    Http2Setting(uint16_t id, uint32_t val) : identifier(htons(id)), value(htonl(val)) {}
};

struct Http2GoAwayPayload {
    uint32_t last_stream_id;  // 31-bit stream ID + 1 reserved bit
    uint32_t error_code;

    Http2GoAwayPayload(uint32_t stream_id, uint32_t error)
        : last_stream_id(htonl(stream_id)), error_code(htonl(error)) {}
};
#pragma pack(pop)
