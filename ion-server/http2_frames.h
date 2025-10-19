#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct Http2FrameHeader {
    uint8_t length[3];    // 24-bit length (big-endian)
    uint8_t type;
    uint8_t flags;
    //uint8_t reserved : 1;
    uint32_t stream_id : 32;   // Stream identifier (big-endian)

    void set_length(uint32_t len) {
        length[0] = (len >> 16) & 0xFF;
        length[1] = (len >> 8) & 0xFF;
        length[2] = len & 0xFF;
    }

    int get_length() const {
        return (length[0] << 16) | (length[1] << 8) | length[2];
    }

    void set_stream_id(uint32_t id) {
        stream_id = htonl(id);
    }
};

struct Http2WindowUpdate {
    uint8_t reserved : 1;
    uint32_t window_size_increment : 31;
};

struct Http2Setting {
    uint16_t identifier;
    uint32_t value;

    Http2Setting(uint16_t id, uint32_t val)
        : identifier(htons(id)), value(htonl(val)) {}
};
#pragma pack(pop)
