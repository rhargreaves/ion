#include "tls_conn.h"

#include <netinet/in.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

TlsConnection::TlsConnection(uint16_t port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::system_error(errno, std::system_category(), "socket");
    }

    constexpr int enable_opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable_opt, sizeof(enable_opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(server_fd);
        throw std::system_error(errno, std::system_category(), "bind");
    }

    client_fd = 0;
    ssl = nullptr;
}

TlsConnection::~TlsConnection() {
    close();
}

// ReSharper disable once CppDFAConstantFunctionResult
int TlsConnection::alpn_callback(SSL*, const unsigned char** out, unsigned char* outlen,
                  const unsigned char* in, unsigned int inlen, void*) {
    static constexpr std::array<unsigned char,3> supported_protos{'\x02', 'h', '2'};

    // Select a protocol from the client's list
    const int result = SSL_select_next_proto(
        const_cast<unsigned char**>(out), outlen,
        supported_protos.data(), supported_protos.size(),
        in, inlen
    );

    if (result == OPENSSL_NPN_NEGOTIATED) {
        std::cout << "ALPN negotiated: ";
        std::cout.write(reinterpret_cast<const char*>(*out), *outlen) << std::endl;
        return SSL_TLSEXT_ERR_OK;
    }

    // No match found - use h2
    *outlen = supported_protos[0];
    *out = supported_protos.data() + 1;
    std::cout << "ALPN negotiation failed, using default: ";
    std::cout.write(reinterpret_cast<const char*>(*out), *outlen) << std::endl;
    return SSL_TLSEXT_ERR_OK;
}

void TlsConnection::listen() const {
    if (::listen(server_fd, 1) < 0) {
        throw std::system_error(errno, std::system_category(), "listen");
    }
}

void TlsConnection::accept() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    const int tmp_client_fd = ::accept(server_fd,
        reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (tmp_client_fd < 0) {
        throw std::system_error(errno, std::system_category(), "accept");
    }
    client_fd = tmp_client_fd;
}

void TlsConnection::handshake(const std::filesystem::path& cert_path,
    const std::filesystem::path& key_path) {

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        throw std::runtime_error("Failed to create SSL context");
    }

    if (SSL_CTX_use_certificate_file(ctx, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("Failed to load certificate file");
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("Failed to load private key file");
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("Private key does not match certificate");
    }

    SSL_CTX_set_alpn_select_cb(ctx, alpn_callback, nullptr);

    SSL_CTX_set_keylog_callback(ctx, [](const SSL*, const char* line) {
        if (auto keylog_file = std::getenv("SSLKEYLOGFILE")) {
            std::ofstream file(keylog_file, std::ios::app);
            if (file.is_open()) {
               file << line << '\n';
            }
        }
    });

    ssl = SSL_new(ctx);
    if (!ssl) {
        SSL_CTX_free(ctx);
        throw std::runtime_error("Failed to create SSL object");
    }

    if (SSL_set_fd(ssl, client_fd) != 1) {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        throw std::runtime_error("Failed to set SSL file descriptor");
    }

    if (SSL_accept(ssl) <= 0) {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        throw std::runtime_error("SSL handshake failed");
    }
}

void TlsConnection::print_debug_to_stderr() {
    ERR_print_errors_fp(stderr);
}

ssize_t TlsConnection::read(std::span<char> buffer) const {
    const auto bytes_read = SSL_read(ssl, buffer.data(), static_cast<int>(buffer.size()));
    if (bytes_read < 0) {
        const int ssl_error = SSL_get_error(ssl, bytes_read);
        throw std::runtime_error("SSL read error: " + std::to_string(ssl_error));
    }
    return bytes_read;
}

ssize_t TlsConnection::write(const std::span<const char> buffer) const {
    const auto bytes_written = SSL_write(ssl, buffer.data(), static_cast<int>(buffer.size()));
    if (bytes_written < 0) {
        const int ssl_error = SSL_get_error(ssl, bytes_written);
        throw std::runtime_error("SSL write error: " + std::to_string(ssl_error));
    }
    return bytes_written;
}

void TlsConnection::close() {
    if (ssl) {
        BIO_flush(SSL_get_wbio(ssl));
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
    if (client_fd > 0) {
        ::close(client_fd);
        client_fd = 0;
    }
    if (server_fd > 0) {
        ::close(server_fd);
        server_fd = 0;
    }
}
