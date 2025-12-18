#include "http2_frame_reader.h"

#include <spdlog/spdlog.h>

#include <expected>

#include "frame_error.h"

namespace ion {

std::expected<std::vector<Http2Setting>, FrameError> Http2FrameReader::read_settings() const {
    if (payload_.size() % Http2Setting::wire_size != 0) {
        spdlog::error("invalid SETTINGS frame: data size not a multiple of {}",
                      Http2Setting::wire_size);
        return std::unexpected(FrameError::InvalidSettingsSize);
    }

    std::vector<Http2Setting> settings;
    const auto num_settings = payload_.size() / Http2Setting::wire_size;
    settings.reserve(num_settings);

    for (size_t i = 0; i < num_settings; ++i) {
        std::span<const uint8_t, Http2Setting::wire_size> entry{
            payload_.data() + i * Http2Setting::wire_size, Http2Setting::wire_size};
        settings.push_back(Http2Setting::parse(entry));
    }

    return settings;
}

Http2WindowUpdate Http2FrameReader::read_window_update() const {
    return Http2WindowUpdate::parse(payload_.subspan<0, Http2WindowUpdate::wire_size>());
}

Http2GoAwayPayload Http2FrameReader::read_goaway() const {
    return Http2GoAwayPayload::parse(payload_.subspan<0, Http2GoAwayPayload::wire_size>());
}

}  // namespace ion
