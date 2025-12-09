#pragma once

#include <csignal>

#include "http2_conn.h"
#include "router.h"
#include "server_config.h"

namespace ion {

class Http2Server {
   public:
    void start(uint16_t port);
    void stop();

    Http2Server() : config_(ServerConfiguration::from_env()) {}
    explicit Http2Server(const ServerConfiguration& config) : config_(config) {}

    Router& router() {
        return router_;
    }

   private:
    volatile std::sig_atomic_t user_req_termination_ = 0;
    Router router_{};
    ServerConfiguration config_;
};

}  // namespace ion
