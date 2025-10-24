#pragma once
#include <openssl/ssl.h>
#include <stdexcept>

class OpenSslContext {
public:
    OpenSslContext() : ctx_(SSL_CTX_new(TLS_server_method())) {
        if (!ctx_) {
            throw std::runtime_error("Failed to create SSL context");
        }
    }

    ~OpenSslContext() {
        if (ctx_) {
            SSL_CTX_free(ctx_);
        }
    }

    // Non-copyable, non-movable
    OpenSslContext(const OpenSslContext&) = delete;
    OpenSslContext& operator=(const OpenSslContext&) = delete;
    OpenSslContext(OpenSslContext&&) = delete;
    OpenSslContext& operator=(OpenSslContext&&) = delete;

    // Conversion to SSL_CTX* for C APIs
    operator SSL_CTX*() const noexcept { return ctx_; }
    SSL_CTX* get() const noexcept { return ctx_; }

private:
    SSL_CTX* ctx_;
};

class OpenSslConnection {
public:
    explicit OpenSslConnection(SSL_CTX* ctx) : ssl_(SSL_new(ctx)) {
        if (!ssl_) {
            throw std::runtime_error("Failed to create SSL object");
        }
    }

    ~OpenSslConnection() {
        if (ssl_) {
            SSL_free(ssl_);  // This also calls SSL_shutdown if needed
        }
    }

    // Non-copyable, non-movable
    OpenSslConnection(const OpenSslConnection&) = delete;
    OpenSslConnection& operator=(const OpenSslConnection&) = delete;
    OpenSslConnection(OpenSslConnection&&) = delete;
    OpenSslConnection& operator=(OpenSslConnection&&) = delete;

    // Conversion to SSL* for C APIs
    operator SSL*() const noexcept { return ssl_; }
    SSL* get() const noexcept { return ssl_; }

private:
    SSL* ssl_;
};
