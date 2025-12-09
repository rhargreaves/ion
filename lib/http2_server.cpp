#include "http2_server.h"

#include <spdlog/spdlog.h>

#include "tcp_listener.h"
#include "tls_conn.h"

namespace ion {

void Http2Server::start(uint16_t port) {
    TcpListener listener{port};
    listener.listen();
    spdlog::info("listening on port {}", port);

    while (!user_req_termination_) {
        auto tcp_conn = listener.try_accept();
        if (!tcp_conn) {
            continue;
        }
        spdlog::info("client connected");

        TlsConnection tls_conn{tcp_conn.value(), config_.cert_path, config_.key_path};
        if (!tls_conn.handshake()) {
            spdlog::info("handshake failed, dropping connection");
            continue;
        }
        spdlog::info("SSL handshake completed successfully");

        Http2Connection http{tls_conn, router_};
        bool connection_valid = true;

        while (!user_req_termination_ && connection_valid) {
            switch (http.process_state()) {
                case Http2ProcessResult::Incomplete:
                    // TODO: retry, mindful of timing out if things are taking too long
                    break;
                case Http2ProcessResult::Complete:
                    // TODO: reset timeout clock
                    break;
                case Http2ProcessResult::ClientClosed:
                    spdlog::info("client closed connection");
                    connection_valid = false;
                    break;
            }
        }
    }

    spdlog::info("server shutting down...");
}

void Http2Server::stop() {
    user_req_termination_ = 1;
}

}  // namespace ion
