#pragma once

#include <csignal>
#include <map>

#include "http2_conn.h"
#include "pollers/poller.h"
#include "router.h"
#include "server_config.h"
#include "stop_reason.h"
#include "tcp_listener.h"
#include "transports/tls_context.h"

namespace ion {
class Poller;

class Http2Server {
   public:
    void start(uint16_t port);
    void stop(StopReason reason = StopReason::Other);

    explicit Http2Server(const ServerConfiguration& config);

    Router& router() {
        return router_;
    }

   private:
    void establish_conn(TcpListener& listener, Poller& poller);
    void reap_idle_connections(Poller& poller);
    void handle_incoming_connection(TcpListener& listener, Poller& poller);
    void handle_connection_events(Poller& poller, int fd, PollEventType poll_events);
    std::unique_ptr<Transport> create_transport(SocketFd&& fd) const;

    volatile std::sig_atomic_t user_req_termination_ = 0;
    Router router_{};
    ServerConfiguration config_;
    StopReason stop_reason_{};
    std::map<int, std::unique_ptr<Http2Connection>> connections_;
    std::optional<TlsContext> tls_ctx_{};
};

}  // namespace ion
