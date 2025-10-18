#pragma once
#include <openssl/ssl.h>
#include <unistd.h>
#include <filesystem>
#include <span>

class TlsConnection {
public:
    explicit TlsConnection(uint16_t port);
    ~TlsConnection();

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