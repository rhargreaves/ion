#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "http2_conn.h"
#include "ion_log_sink.h"
#include "server.h"

namespace ion::wasm {

static std::unique_ptr<Server> g_server;

__attribute__((export_name("ion_init"))) extern "C" void ion_init() {
    const char msg[] = "ion_init: hello from wasm\n";
    ion_log(2, msg, sizeof(msg) - 1);

    auto logger = std::make_shared<spdlog::logger>("ion", std::make_shared<IonLogSink>());
    logger->set_level(spdlog::level::trace);
    logger->info("ion_init: logger initialised");
    spdlog::set_default_logger(std::move(logger));

    g_server = std::make_unique<Server>();
}

__attribute__((export_name("ion_conn_create"))) extern "C" int ion_conn_create() {
    return g_server ? g_server->create_connection() : -1;
}

__attribute__((export_name("ion_conn_feed"))) extern "C" void ion_conn_feed(int id,
                                                                            const uint8_t* data,
                                                                            std::size_t len) {
    if (g_server)
        g_server->feed(id, {data, len});
}

__attribute__((export_name("ion_conn_process"))) extern "C" int ion_conn_process(int id) {
    return g_server ? g_server->process(id) : 1;
}

__attribute__((export_name("ion_conn_output_size"))) extern "C" std::size_t ion_conn_output_size(
    int id) {
    return g_server ? g_server->output_size(id) : 0;
}

__attribute__((export_name("ion_conn_output_read"))) extern "C" std::size_t ion_conn_output_read(
    int id, uint8_t* out, std::size_t max) {
    return g_server ? g_server->output_read(id, {out, max}) : 0;
}

__attribute__((export_name("ion_conn_close"))) extern "C" void ion_conn_close(int id) {
    if (g_server)
        g_server->close(id);
}

}  // namespace ion::wasm
