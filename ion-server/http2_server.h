#pragma once

#include <csignal>

#include "http2_conn.h"

class Http2Server {
   public:
    void run_server(uint16_t port);
    void stop_server();

   private:
    volatile std::sig_atomic_t should_stop_ = 0;
};
