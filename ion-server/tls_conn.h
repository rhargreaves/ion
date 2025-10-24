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
    static void print_debug_to_stderr();
    ssize_t read(std::span<uint8_t> buffer) const;
    ssize_t write(std::span<const uint8_t> buffer) const;
    [[nodiscard]] bool has_data() const;

   private:
    int server_fd = -1;
    int client_fd = -1;
    SSL* ssl = nullptr;

    static int alpn_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
                             const unsigned char* in, unsigned int inlen, void* arg);
};
