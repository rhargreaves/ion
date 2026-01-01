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
static constexpr std::chrono::milliseconds POLL_TIMEOUT{25};

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
    return std::make_unique<TlsTransport>(std::move(fd), *tls_ctx_);
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

void Http2Server::reap_idle_connections(Poller& poller) {
    for (auto it = connections_.begin(); it != connections_.end();) {
        const auto& conn = it->second;
        if (conn->has_timed_out()) {
            spdlog::warn("closing connection due to inactivity (fd: {})", it->first);
            poller.remove(it->first);
            it = connections_.erase(it);
        } else {
            ++it;
        }
    }
}

void Http2Server::handle_incoming_connection(TcpListener& listener, Poller& poller) {
    const bool at_capacity = connections_.size() >= MAX_CONNECTIONS;
    if (!at_capacity) {
        establish_conn(listener, poller);
    } else {
        if (const auto overflow = listener.try_accept()) {
            spdlog::warn("server at capacity. rejecting new connection from {}",
                         overflow->client_ip().value_or("unknown IP"));
        }
    }
}

void Http2Server::handle_connection_events(Poller& poller, int fd, PollEventType poll_events) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        return;
    }
    if (has_event(poll_events, PollEventType::Error) ||
        has_event(poll_events, PollEventType::Hangup)) {
        spdlog::debug("poll indicated connection closed");
        connections_.erase(it);
        poller.remove(fd);
        return;
    }

    if (!has_event(poll_events, PollEventType::Read) &&
        !has_event(poll_events, PollEventType::Write)) {
        return;
    }

    // assume we are not write blocked until WantWrite occurs again
    if (has_event(poll_events, PollEventType::Write)) {
        poller.set(fd, PollEventType::Read);
    }

    const auto& conn = it->second;
    switch (conn->process()) {
        case Http2ProcessResult::WantWrite:
            spdlog::trace("will poll write events for fd {}", fd);
            poller.set(fd, PollEventType::Read | PollEventType::Write);
            break;
        case Http2ProcessResult::WantRead:
            // interest is already Read (or was reset above), just keep waiting
            break;
        case Http2ProcessResult::DiscardConnection:
            spdlog::info("closing connection");
            poller.remove(fd);
            connections_.erase(it);
            break;
    }
}

void Http2Server::start(uint16_t port) {
    TcpListener listener{port};
    listener.listen();
    const int listener_fd = listener.raw_fd();
    spdlog::info("listening on port {}", port);

    const auto poller = Poller::create();
    poller->set(listener_fd, PollEventType::Read);

    while (!user_req_termination_) {
        reap_idle_connections(*poller);

        auto events = poller->poll(POLL_TIMEOUT);
        if (!events) {
            continue;
        }
        for (auto& [fd, poll_events] : *events) {
            if (fd == listener_fd) {
                handle_incoming_connection(listener, *poller);
            } else {
                handle_connection_events(*poller, fd, poll_events);
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
