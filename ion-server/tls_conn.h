#pragma once
#include <openssl/ssl.h>
#include <unistd.h>

#include <filesystem>
#include <span>

#include "socket_fd.h"

class TlsConnection {
   public:
    explicit TlsConnection(uint16_t port);
    ~TlsConnection();

    void listen() const;
    void accept();
    void close();
    void handshake(const std::filesystem::path& cert_path, const std::filesystem::path& key_path);
    static void print_debug_to_stderr();
    ssize_t read(std::span<uint8_t> buffer) const;
    ssize_t write(std::span<const uint8_t> buffer) const;
    [[nodiscard]] bool has_data() const;

   private:
    SocketFd server_fd_;
    SocketFd client_fd_;
    SSL* ssl_ = nullptr;

    static int alpn_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
                             const unsigned char* in, unsigned int inlen, void* arg);

    void set_nonblocking_socket();
    void set_reusable_addr();
    void bind_socket(uint16_t port);
};
