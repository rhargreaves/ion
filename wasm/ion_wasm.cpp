#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "ion_log_sink.h"

namespace ion::wasm {

extern "C" void ion_init() {
    const char msg[] = "ion_init: hello from wasm\n";
    ion_log(2, msg, sizeof(msg) - 1);

    auto logger = std::make_shared<spdlog::logger>("ion", std::make_shared<IonLogSink>());
    logger->set_level(spdlog::level::trace);
    logger->info("ion_init: logger initialised");

    spdlog::set_default_logger(std::move(logger));
}

}  // namespace ion::wasm
