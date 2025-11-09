#pragma once

#include <csignal>

#include "http2_conn.h"

namespace ion {

class Http2Server {
   public:
    void run_server(uint16_t port);
    void stop_server();

   private:
    volatile std::sig_atomic_t user_req_termination_ = 0;
};

}  // namespace ion
