#include "http2_server.h"

#include "http2_except.h"
#include "spdlog/spdlog.h"
#include "tls_conn.h"
#include "tls_conn_except.h"

void Http2Server::run_server(uint16_t port) {
    TcpConnection tcp_conn{port};
    tcp_conn.listen();
    spdlog::info("listening on port {}", port);

    while (!should_stop_) {
        try {
            if (!tcp_conn.try_accept()) {
                continue;
            }
            spdlog::info("client connected");

            TlsConnection tls_conn{tcp_conn, "cert.pem", "key.pem"};
            tls_conn.handshake();
            spdlog::info("SSL handshake completed successfully");

            Http2Connection http{tls_conn};
            while (!should_stop_) {
                switch (http.process_state()) {
                    case Http2ProcessResult::AwaitingData:
                    case Http2ProcessResult::ProcessedData:
                        break;
                    case Http2ProcessResult::ClosingCleanly:
                        should_stop_ = 2;
                        break;
                    default:
                        throw std::runtime_error("invalid state");
                }
            }
            tls_conn.graceful_shutdown();

        } catch (const TlsConnectionClosed& e) {
            spdlog::info("connection closed (exception): {}", e.what());
        } catch (const Http2TimeoutException& e) {
            spdlog::info("connection closed (timeout): {}", e.what());
        } catch (const std::exception& e) {
            spdlog::error(e.what());
            throw;
        }
    }

    if (should_stop_) {
        spdlog::info("server shutting down...");
    }
}

void Http2Server::stop_server() { should_stop_ = 1; }
