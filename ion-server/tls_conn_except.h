#pragma once
#include <exception>
#include <string>

class TlsConnectionClosed : public std::exception {
   public:
    enum class Reason { CLEAN_SHUTDOWN, CONNECTION_RESET, BROKEN_PIPE, SOCKET_ERROR };

    explicit TlsConnectionClosed(Reason reason) {
        switch (reason) {
            case Reason::CLEAN_SHUTDOWN:
                message_ = "TLS connection closed cleanly by peer";
                break;
            case Reason::CONNECTION_RESET:
                message_ = "TLS connection reset by peer";
                break;
            case Reason::BROKEN_PIPE:
                message_ = "TLS connection broken pipe";
                break;
            case Reason::SOCKET_ERROR:
                message_ = "TLS connection socket error";
                break;
        }
    }

    const char* what() const noexcept override { return message_.c_str(); }

   private:
    std::string message_;
};
