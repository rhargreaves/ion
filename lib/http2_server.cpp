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

void Http2Server::establish_conn(TcpListener& listener) {
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
}

void Http2Server::start(uint16_t port) {
    TcpListener listener{port};
    listener.listen();
    const int listener_fd = listener.raw_fd();
    spdlog::info("listening on port {}", port);
    std::vector<pollfd> poll_fds{};
    std::set<int> want_write_fds{};

    while (!user_req_termination_) {
        poll_fds.clear();

        // if not at capacity - accept new connections
        const bool at_capacity = connections_.size() >= MAX_CONNECTIONS;
        if (!at_capacity) {
            poll_fds.push_back({listener_fd, POLLIN, 0});
        }

        // prepare pollfd array with HTTP/2 connection states
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
            if (errno == EINTR) {
                spdlog::warn("poll interrupted by signal");
                continue;
            }
            spdlog::error("poll failed: {}", strerror(errno));
            continue;
        }

        // dispatch events
        for (auto& pfd : poll_fds) {
            if (pfd.revents == 0) {
                continue;
            }

            if (pfd.fd == listener_fd) {
                // new client
                establish_conn(listener);
                continue;
            }

            // existing connection?
            auto it = connections_.find(pfd.fd);
            if (it == connections_.end()) {
                continue;
            }
            if (pfd.revents & (POLLERR | POLLHUP)) {
                spdlog::debug("connection closed");
                connections_.erase(it);
                continue;
            }
            if (!(pfd.revents & (POLLIN | POLLOUT))) {
                continue;
            }

            // assume we aren't write blocked until WantWrite occurs again
            want_write_fds.erase(pfd.fd);

            bool conn_active = true;
            while (conn_active) {
                const auto& conn = it->second;
                switch (conn->process()) {
                    case Http2ProcessResult::WantWrite:
                        want_write_fds.insert(pfd.fd);
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
                        conn_active = false;
                        break;
                    case Http2ProcessResult::ProtocolError:
                        spdlog::error("protocol error. closing connection");
                        conn->close();
                        connections_.erase(it);
                        conn_active = false;
                        break;
                    case Http2ProcessResult::ConnectionError:
                        spdlog::error("connection error. force closing connection");
                        connections_.erase(it);
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
