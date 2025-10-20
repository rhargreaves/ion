#pragma once
#include <arpa/inet.h>

#include <span>

inline uint32_t load_uint24_be(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 16) | (static_cast<uint32_t>(data[1]) << 8) |
           static_cast<uint32_t>(data[2]);
}

inline uint32_t load_uint32_be(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
}

inline void store_uint24_be(uint32_t value, uint8_t* data) {
    data[0] = (value >> 16) & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;
}

inline void store_uint32_be(uint32_t value, uint8_t* data) {
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
}

struct Http2FrameHeader {
    uint32_t length;
    uint8_t type;
    uint8_t flags;
    uint32_t stream_id;

    static Http2FrameHeader parse(std::span<const uint8_t, 9> data) {
        return Http2FrameHeader{.length = load_uint24_be(data.data()),
                                .type = data[3],
                                .flags = data[4],
                                .stream_id = load_uint32_be(data.data() + 5) & 0x7FFFFFFF};
    }

    void serialize(std::span<uint8_t, 9> data) const {
        store_uint24_be(length, data.data());
        data[3] = type;
        data[4] = flags;
        store_uint32_be(stream_id & 0x7FFFFFFF, data.data() + 5);
    }
};

#pragma pack(push, 1)
struct Http2WindowUpdate {
    uint32_t reserved_and_window_size_increment;
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
