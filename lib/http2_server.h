#pragma once

#include <csignal>
#include <map>

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

    explicit Http2Server(const ServerConfiguration& config);

    Router& router() {
        return router_;
    }

   private:
    void establish_conn(TcpListener& listener);
    std::unique_ptr<Transport> create_transport(SocketFd&& fd) const;

    volatile std::sig_atomic_t user_req_termination_ = 0;
    Router router_{};
    ServerConfiguration config_;
    StopReason stop_reason_{};
    std::map<int, std::unique_ptr<Http2Connection>> connections_;
};

}  // namespace ion
