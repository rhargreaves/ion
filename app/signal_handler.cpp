#include "signal_handler.h"

#include <unistd.h>

#include <csignal>

SignalHandler SignalHandler::setup(ion::Http2Server& server) {
    ion::Http2Server* expected = nullptr;
    if (!server_.compare_exchange_strong(expected, &server)) {
        throw std::runtime_error("SignalHandler has already been initialized!");
    }
    return SignalHandler(server);
}

SignalHandler::SignalHandler(ion::Http2Server& server) {
    server_.store(&server);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, sigpipe_handler);
}

SignalHandler::~SignalHandler() {
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
    std::signal(SIGPIPE, SIG_DFL);
    server_.store(nullptr);
};

void SignalHandler::stop_server(ion::StopReason reason) {
    if (ion::Http2Server* server = server_.load()) {
        server->stop(reason);
    }
}

void SignalHandler::signal_handler(int) {
    stop_server(ion::StopReason::Signal);
}

void SignalHandler::sigpipe_handler(int) {
    constexpr char dummy = 0;
    if (write(STDOUT_FILENO, &dummy, 0) < 0 && errno == EPIPE) {
        stop_server(ion::StopReason::StdOutPipeBroken);
    }
    if (write(STDERR_FILENO, &dummy, 0) < 0 && errno == EPIPE) {
        stop_server(ion::StopReason::StdErrPipeBroken);
    }
}
