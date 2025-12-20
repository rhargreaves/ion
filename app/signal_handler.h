#pragma once
#include <atomic>

#include "http2_server.h"

class SignalHandler {
   public:
    static SignalHandler setup(ion::Http2Server& server);
    ~SignalHandler();

    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;
    SignalHandler(SignalHandler&&) = delete;

   private:
    explicit SignalHandler(ion::Http2Server& server);
    static void stop_server(ion::StopReason reason);
    static void signal_handler(int);
    static void sigpipe_handler(int);

    static inline std::atomic<ion::Http2Server*> server_{nullptr};
};
