#include "http2_server.h"

#include <spdlog/spdlog.h>

#include <optional>
#include <set>

#include "pollers/poll_poller.h"
#include "tcp_listener.h"
#include "transports/tcp_transport.h"
#include "transports/tls_context.h"
#include "transports/tls_transport.h"

namespace ion {

static constexpr std::size_t MAX_CONNECTIONS = 128;

Http2Server::Http2Server(const ServerConfiguration& config) : router_(Router{}), config_(config) {
    config_.validate();
    if (!config_.cleartext) {
        tls_ctx_.emplace(*config_.cert_path, *config_.key_path);
    }
}

std::unique_ptr<Transport> Http2Server::create_transport(SocketFd&& fd) const {
    if (config_.cleartext) {
        return std::make_unique<TcpTransport>(std::move(fd));
    }

    auto tls = std::make_unique<TlsTransport>(std::move(fd), *tls_ctx_);
    if (!tls->handshake()) {
        return nullptr;
    }
    spdlog::debug("TLS handshake completed successfully");
    return tls;
}

void Http2Server::establish_conn(TcpListener& listener, Poller& poller) {
    auto fd = listener.try_accept();
    if (!fd) {
        return;
    }
    const auto client_ip_res = fd->client_ip();
    spdlog::info("client connected (ip: {})", client_ip_res.value_or("unknown"));

    const int raw_fd = *fd;
    auto transport = create_transport(std::move(fd.value()));
    if (!transport) {
        return;
    }

    auto conn = std::make_unique<Http2Connection>(std::move(transport), client_ip_res.value_or(""),
                                                  router_);
    connections_[raw_fd] = std::move(conn);
    spdlog::info("HTTP connection established. total = {}", connections_.size());

    poller.set(raw_fd, PollEventType::Read);
}

void Http2Server::start(uint16_t port) {
    TcpListener listener{port};
    listener.listen();
    const int listener_fd = listener.raw_fd();
    spdlog::info("listening on port {}", port);

    auto poller = Poller::create();
    poller->set(listener_fd, PollEventType::Read);

    while (!user_req_termination_) {
        auto events = poller->poll(std::chrono::milliseconds{100});
        if (!events) {
            continue;
        }

        // dispatch events
        for (auto& [fd, poll_events] : *events) {
            if (fd == listener_fd) {
                // new client
                const bool at_capacity = connections_.size() >= MAX_CONNECTIONS;
                if (!at_capacity) {
                    establish_conn(listener, *poller);
                } else {
                    if (const auto overflow = listener.try_accept()) {
                        spdlog::warn("server at capacity. rejecting new connection from {}",
                                     overflow->client_ip().value_or("unknown IP"));
                    }
                }
                continue;
            }

            // existing connection?
            auto it = connections_.find(fd);
            if (it == connections_.end()) {
                continue;
            }
            if (has_event(poll_events, PollEventType::Error) ||
                has_event(poll_events, PollEventType::Hangup)) {
                spdlog::debug("connection closed");
                connections_.erase(it);
                poller->remove(fd);
                continue;
            }

            if (!has_event(poll_events, PollEventType::Read) &&
                !has_event(poll_events, PollEventType::Write)) {
                continue;
            }

            // assume we are not write blocked until WantWrite occurs again
            if (has_event(poll_events, PollEventType::Write)) {
                poller->set(fd, PollEventType::Read);
            }

            bool conn_active = true;
            while (conn_active) {
                const auto& conn = it->second;
                switch (conn->process()) {
                    case Http2ProcessResult::WantWrite:
                        spdlog::trace("will poll write events for fd {}", fd);
                        poller->set(fd, PollEventType::Read | PollEventType::Write);
                        conn_active = false;
                        break;
                    case Http2ProcessResult::WantRead:
                        // rely on poll() to tell us when new data is available
                        conn_active = false;
                        break;
                    case Http2ProcessResult::Complete:
                        // need to keep processing until want read/write is hit
                        break;
                    case Http2ProcessResult::ClientClosed:
                        spdlog::info("client closed connection");
                        connections_.erase(it);
                        poller->remove(fd);
                        conn_active = false;
                        break;
                    case Http2ProcessResult::ProtocolError:
                        spdlog::error("protocol error. closing connection");
                        conn->close();
                        connections_.erase(it);
                        poller->remove(fd);
                        conn_active = false;
                        break;
                    case Http2ProcessResult::ConnectionError:
                        spdlog::error("connection error. force closing connection");
                        connections_.erase(it);
                        poller->remove(fd);
                        conn_active = false;
                        break;
                }
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
