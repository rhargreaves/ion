#pragma once

#include <csignal>

#include "http2_conn.h"
#include "router.h"
#include "server_config.h"
#include "stop_reason.h"
#include "tcp_listener.h"

namespace ion {

class Http2Server {
   public:
    void start(uint16_t port);
    void stop(StopReason reason = StopReason::Other);

    Http2Server();
    explicit Http2Server(const ServerConfiguration& config);

    Router& router() {
        return router_;
    }

   private:
    std::unique_ptr<Http2Connection> try_establish_conn(TcpListener& listener,
                                                        std::chrono::milliseconds timeout);
    std::unique_ptr<Transport> create_transport(SocketFd&& fd) const;

    volatile std::sig_atomic_t user_req_termination_ = 0;
    Router router_{};
    ServerConfiguration config_;
    StopReason stop_reason_{};
};

}  // namespace ion
