#include "signal_handler.h"

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

void SignalHandler::stop_server() {
    if (ion::Http2Server* server = server_.load()) {
        server->stop();
    }
}

void SignalHandler::signal_handler(int) {
    spdlog::info("stopping server (signal)...");
    stop_server();
}

void SignalHandler::sigpipe_handler(int) {
    char dummy;

    if (write(STDOUT_FILENO, &dummy, 0) < 0 && errno == EPIPE) {
        spdlog::error("stdout pipe broken, terminating");
        stop_server();
    }

    if (write(STDERR_FILENO, &dummy, 0) < 0 && errno == EPIPE) {
        spdlog::error("stderr pipe broken, terminating");
        stop_server();
    }

    spdlog::debug("SIGPIPE from network connection, ignoring");
}
