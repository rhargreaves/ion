#pragma once

#include <spdlog/common.h>
#include <spdlog/sinks/base_sink.h>

__attribute__((import_module("env"), import_name("ion_log"))) extern "C" void ion_log(
    int level, const char* msg, unsigned long len);

namespace ion::wasm {

class IonLogSink final : public spdlog::sinks::base_sink<spdlog::details::null_mutex> {
   protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        ion_log(static_cast<int>(msg.level), formatted.data(), formatted.size());
    }
    void flush_() override {}
};

}  // namespace ion::wasm
