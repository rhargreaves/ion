#include "system_clock.h"

#include <chrono>

std::string SystemClock::clf_timestamp() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm loc_tm;
    localtime_r(&now, &loc_tm);

    std::string buffer(32, '\0');
    const size_t len =
        std::strftime(buffer.data(), buffer.size(), "[%d/%b/%Y:%H:%M:%S %z]", &loc_tm);
    buffer.resize(len);
    return buffer;
}
