#include "http2_server.h"

#include <poll.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <set>
#include <vector>

#include "tcp_listener.h"
#include "transports/tcp_transport.h"
#include "transports/tls_transport.h"

namespace ion {

static constexpr std::size_t MAX_CONNECTIONS = 128;

Http2Server::Http2Server(const ServerConfiguration& config) : config_(config), router_(Router{}) {
    config_.validate();
}

std::unique_ptr<Transport> Http2Server::create_transport(SocketFd&& fd) const {
    if (config_.cleartext) {
        return std::make_unique<TcpTransport>(std::move(fd));
    }

    auto tls = std::make_unique<TlsTransport>(std::move(fd), *config_.cert_path, *config_.key_path);
    if (!tls->handshake()) {
        return nullptr;
    }
    spdlog::debug("TLS handshake completed successfully");
    return tls;
}

std::unique_ptr<Http2Connection> Http2Server::try_establish_conn(
    TcpListener& listener, std::chrono::milliseconds timeout) {
    auto fd = listener.try_accept(timeout);
    if (!fd) {
        return nullptr;
    }
    const auto client_ip_res = fd->client_ip();
    spdlog::info("client connected (ip: {})", client_ip_res.value_or("unknown"));

    const int raw_fd = *fd;
    auto transport = create_transport(std::move(fd.value()));
    if (!transport) {
        return nullptr;
    }

    auto conn = std::make_unique<Http2Connection>(std::move(transport), client_ip_res.value_or(""),
                                                  router_);
    connections_[raw_fd] = std::move(conn);
    spdlog::info("HTTP connection established. total = {}", connections_.size());

    return conn;
}

void Http2Server::start(uint16_t port) {
    TcpListener listener{port};
    listener.listen();
    spdlog::info("listening on port {}", port);
    std::vector<pollfd> poll_fds;
    std::set<int> want_write_fds{};

    while (!user_req_termination_) {
        // if not at capacity - accept new connections
        const bool at_capacity = connections_.size() >= MAX_CONNECTIONS;
        if (!at_capacity) {
            auto conn = try_establish_conn(
                listener, std::chrono::milliseconds{connections_.empty() ? 100 : 0});
        }

        // prepare pollfd array with HTTP/2 connection states
        poll_fds.clear();
        for (auto& [fd, conn] : connections_) {
            short events = POLLIN;
            if (want_write_fds.count(fd) > 0) {
                spdlog::trace("fd {} is in write set", fd);
                events |= POLLOUT;
            }
            spdlog::trace("adding fd {} to poll list", fd);
            poll_fds.push_back({fd, events, 0});
        }

        // wait for activity
        const int ret = poll(poll_fds.data(), poll_fds.size(), 100);
        if (ret == 0) {
            spdlog::trace("poll timed out");
            continue;
        }
        if (ret < 0) {
            spdlog::error("poll failed: {}", strerror(errno));
            continue;
        }

        // dispatch events
        for (auto& pfd : poll_fds) {
            if (pfd.revents == 0) {
                continue;
            }

            auto it = connections_.find(pfd.fd);
            if (it == connections_.end()) {
                continue;
            }
            if (pfd.revents & (POLLERR | POLLHUP)) {
                spdlog::debug("connection closed");
                connections_.erase(it);
                continue;
            }

            bool trigger = false;
            if (pfd.revents & POLLIN) {
                trigger = true;
            } else if (pfd.revents & POLLOUT) {
                trigger = true;
            }
            if (!trigger) {
                continue;
            }

        repeat:
            const auto& conn = it->second;
            switch (conn->process_state()) {
                case Http2ProcessResult::WantWrite:
                    want_write_fds.insert(pfd.fd);
                    break;
                case Http2ProcessResult::WantRead:
                    // rely on poll() to tell us when new data is available
                    break;
                case Http2ProcessResult::Complete:
                    // need to keep pulling data until want read/write is hit
                    goto repeat;
                case Http2ProcessResult::ClientClosed:
                    spdlog::info("client closed connection");
                    connections_.erase(it);
                    break;
                case Http2ProcessResult::ProtocolError:
                    spdlog::error("protocol error. closing connection");
                    conn->close();
                    connections_.erase(it);
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
