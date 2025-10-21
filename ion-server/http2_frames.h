#pragma once
#include <cstdint>
#include <span>

inline uint16_t load_uint16_be(std::span<const uint8_t, 2> data) {
    return static_cast<uint16_t>(data[0]) << 8 | static_cast<uint16_t>(data[1]);
}

inline uint32_t load_uint24_be(std::span<const uint8_t, 3> data) {
    return static_cast<uint32_t>(data[0]) << 16 | static_cast<uint32_t>(data[1]) << 8 |
           static_cast<uint32_t>(data[2]);
}

inline uint32_t load_uint32_be(std::span<const uint8_t, 4> data) {
    return static_cast<uint32_t>(data[0]) << 24 | static_cast<uint32_t>(data[1]) << 16 |
           static_cast<uint32_t>(data[2]) << 8 | static_cast<uint32_t>(data[3]);
}

inline void store_uint16_be(uint16_t value, std::span<uint8_t, 2> data) {
    data[0] = static_cast<uint8_t>(value >> 8 & 0xFF);
    data[1] = static_cast<uint8_t>(value & 0xFF);
}

inline void store_uint24_be(uint32_t value, std::span<uint8_t, 3> data) {
    data[0] = static_cast<uint8_t>(value >> 16 & 0xFF);
    data[1] = static_cast<uint8_t>(value >> 8 & 0xFF);
    data[2] = static_cast<uint8_t>(value & 0xFF);
}

inline void store_uint32_be(uint32_t value, std::span<uint8_t, 4> data) {
    data[0] = static_cast<uint8_t>(value >> 24 & 0xFF);
    data[1] = static_cast<uint8_t>(value >> 16 & 0xFF);
    data[2] = static_cast<uint8_t>(value >> 8 & 0xFF);
    data[3] = static_cast<uint8_t>(value & 0xFF);
}

struct Http2FrameHeader {
    uint32_t length;
    uint8_t type;
    uint8_t flags;
    uint32_t stream_id;

    static constexpr size_t wire_size = 9;

    static Http2FrameHeader parse(std::span<const uint8_t, wire_size> data) {
        return Http2FrameHeader{.length = load_uint24_be(data.subspan<0, 3>()),
                                .type = data[3],
                                .flags = data[4],
                                .stream_id = load_uint32_be(data.subspan<5, 4>()) & 0x7FFFFFFF};
    }

    void serialize(std::span<uint8_t, wire_size> data) const {
        store_uint24_be(length, data.subspan<0, 3>());
        data[3] = type;
        data[4] = flags;
        store_uint32_be(stream_id & 0x7FFFFFFF, data.subspan<5, 4>());
    }
};

struct Http2Setting {
    uint16_t identifier;
    uint32_t value;

    static constexpr size_t wire_size = 6;

    static Http2Setting parse(std::span<const uint8_t, wire_size> data) {
        return Http2Setting{
            .identifier = load_uint16_be(data.subspan<0, 2>()),
            .value = load_uint32_be(data.subspan<2, 4>()),
        };
    }

    void serialize(std::span<uint8_t, wire_size> data) const {
        store_uint16_be(identifier, data.subspan<0, 2>());
        store_uint32_be(value, data.subspan<2, 4>());
    }
};

struct Http2GoAwayPayload {
    uint32_t last_stream_id;  // 31-bit value (reserved bit stripped)
    uint32_t error_code;

    static constexpr size_t wire_size = 8;

    static Http2GoAwayPayload parse(std::span<const uint8_t, wire_size> data) {
        return Http2GoAwayPayload{
            .last_stream_id = load_uint32_be(data.subspan<0, 4>()) & 0x7FFFFFFF,
            .error_code = load_uint32_be(data.subspan<4, 4>()),
        };
    }

    void serialize(std::span<uint8_t, wire_size> data) const {
        store_uint32_be(last_stream_id & 0x7FFFFFFF, data.subspan<0, 4>());
        store_uint32_be(error_code, data.subspan<4, 4>());
    }
};

struct Http2WindowUpdate {
    uint32_t window_size_increment;  // 31-bit value (reserved bit stripped)

    static constexpr size_t wire_size = 4;

    static Http2WindowUpdate parse(std::span<const uint8_t, wire_size> data) {
        return Http2WindowUpdate{
            .window_size_increment = load_uint32_be(data) & 0x7FFFFFFF,
        };
    }

    void serialize(std::span<uint8_t, wire_size> data) const {
        store_uint32_be(window_size_increment & 0x7FFFFFFF, data);
    }
};
