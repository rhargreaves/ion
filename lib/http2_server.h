#pragma once

#include <csignal>

#include "http2_conn.h"
#include "router.h"

namespace ion {

class Http2Server {
   public:
    void run_server(uint16_t port);
    void stop_server();

    Router& router() {
        return router_;
    }

   private:
    volatile std::sig_atomic_t user_req_termination_ = 0;
    Router router_{};
};

}  // namespace ion
