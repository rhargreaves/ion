#include "http2_server.h"

#include <spdlog/spdlog.h>

#include <optional>
#include <vector>

#include "tcp_listener.h"
#include "tcp_transport.h"
#include "tls_transport.h"

namespace ion {

static constexpr std::size_t MAX_CONNECTIONS = 128;

std::unique_ptr<Http2Connection> Http2Server::try_establish_conn(
    TcpListener& listener, std::chrono::milliseconds timeout) {
    auto fd = listener.try_accept(timeout);
    if (!fd) {
        return nullptr;
    }

    if (config_.cleartext) {
        auto tcp_transport = std::make_unique<TcpTransport>(std::move(fd.value()));
        spdlog::info("client connected (ip: {})", tcp_transport->client_ip().value_or("unknown"));
        return std::make_unique<Http2Connection>(std::move(tcp_transport), router_);
    }

    auto tls_transport =
        std::make_unique<TlsTransport>(std::move(fd.value()), config_.cert_path, config_.key_path);
    spdlog::info("client connected (ip: {})", tls_transport->client_ip().value_or("unknown"));
    if (!tls_transport->handshake()) {
        return nullptr;
    }
    spdlog::debug("SSL handshake completed successfully");
    return std::make_unique<Http2Connection>(std::move(tls_transport), router_);
}

void Http2Server::start(uint16_t port) {
    TcpListener listener{port};
    listener.listen();
    spdlog::info("listening on port {}", port);

    std::vector<std::unique_ptr<Http2Connection>> connections{};
    while (!user_req_termination_) {
        const bool at_capacity = connections.size() >= MAX_CONNECTIONS;

        if (!at_capacity) {
            auto conn = try_establish_conn(
                listener, std::chrono::milliseconds{connections.empty() ? 100 : 0});
            if (conn) {
                connections.emplace_back(std::move(conn));
                spdlog::info("HTTP connection established. total = {}", connections.size());
            }
        }

        for (auto it = connections.begin(); it != connections.end();) {
            auto& http = **it;
            switch (http.process_state()) {
                case Http2ProcessResult::Incomplete:
                    // TODO: retry, mindful of timing out if things are taking too long
                    ++it;
                    break;
                case Http2ProcessResult::Complete:
                    // TODO: reset timeout clock
                    ++it;
                    break;
                case Http2ProcessResult::ClientClosed:
                    spdlog::info("client closed connection");
                    it = connections.erase(it);
                    break;
                case Http2ProcessResult::ProtocolError:
                    spdlog::error("protocol error. closing connection");
                    http.close();
                    it = connections.erase(it);
            }
        }
    }

    spdlog::info("server shutting down (reason: {})", StopReasonHelper::to_string(stop_reason_));
}

void Http2Server::stop(StopReason reason) {
    stop_reason_ = reason;
    user_req_termination_ = 1;
}

}  // namespace ion
