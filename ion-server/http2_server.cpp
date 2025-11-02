#include "http2_server.h"

#include "spdlog/spdlog.h"
#include "tls_conn.h"

void Http2Server::run_server(uint16_t port) {
    TcpConnection tcp_conn{port};
    tcp_conn.listen();
    spdlog::info("listening on port {}", port);

    while (!user_req_termination_) {
        if (!tcp_conn.try_accept()) {
            continue;
        }
        spdlog::info("client connected");

        TlsConnection tls_conn{tcp_conn, "cert.pem", "key.pem"};
        tls_conn.handshake();
        spdlog::info("SSL handshake completed successfully");

        Http2Connection http{tls_conn};
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

void Http2Server::stop_server() { user_req_termination_ = 1; }
