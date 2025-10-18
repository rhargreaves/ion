//
// Created by Robert Hargreaves on 18/10/2025.
//

#ifndef ION_SERVER_TLS_CONN_H
#define ION_SERVER_TLS_CONN_H
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <vector>
#include <span>

class TlsConnection {
public:
    explicit TlsConnection(uint16_t port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw std::system_error(errno, std::system_category(), "socket");
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

    ~TlsConnection() {
        if (server_fd > 0) {
            ::close(server_fd);
        }
        if (client_fd > 0) {
            ::close(client_fd);
        }
        if (ssl) {
            ::SSL_free(ssl);
        }
    }

    void listen() const;
    void accept();
    void close();
    void handshake(const std::filesystem::path& cert_path, const std::filesystem::path& key_path);
    void print_debug_to_stderr();
    ssize_t read(std::span<char> buffer);
    ssize_t write(std::span<const char> buffer);

private:
    int server_fd;
    int client_fd;
    SSL* ssl;

    static int alpn_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
        const unsigned char* in, unsigned int inlen, void* arg);
};

#endif  // ION_SERVER_TLS_CONN_H
