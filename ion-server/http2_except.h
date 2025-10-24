#pragma once
#include <exception>

class Http2Exception : public std::exception {};

class Http2TimeoutException : public Http2Exception {
   public:
    Http2TimeoutException() = default;
    ~Http2TimeoutException() override = default;
};
