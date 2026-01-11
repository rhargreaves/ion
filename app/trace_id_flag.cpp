#include "trace_id_flag.h"

#include <opentelemetry/trace/provider.h>

namespace ion::app {

void TraceIdFlag::format(const spdlog::details::log_msg&, const std::tm&,
                         spdlog::memory_buf_t& dest) {
    auto span = opentelemetry::trace::Tracer::GetCurrentSpan();
    if (span->GetContext().IsValid()) {
        char trace_id[32];
        span->GetContext().trace_id().ToLowerBase16(trace_id);

        dest.push_back('[');
        dest.append(trace_id, trace_id + 32);
        dest.push_back(']');
        dest.push_back(' ');
    }
}
std::unique_ptr<spdlog::custom_flag_formatter> TraceIdFlag::clone() const {
    return std::make_unique<TraceIdFlag>();
}

}  // namespace ion::app
