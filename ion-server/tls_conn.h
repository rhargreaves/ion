#pragma once
#include <openssl/ssl.h>
#include <unistd.h>

#include <expected>
#include <filesystem>
#include <span>

#include "tcp_conn.h"

enum class TlsError { WantReadOrWrite, ConnectionClosed, ProtocolError, OtherError };

class TlsConnection {
   public:
    explicit TlsConnection(TcpConnection& tcp_conn, const std::filesystem::path& cert_path,
                           const std::filesystem::path& key_path);
    ~TlsConnection();

    static void print_debug_to_stderr();
    std::expected<ssize_t, TlsError> read(std::span<uint8_t> buffer) const;
    ssize_t write(std::span<const uint8_t> buffer) const;
    [[nodiscard]] bool has_data() const;
    void handshake() const;
    void graceful_shutdown() const;

   private:
    TcpConnection& tcp_conn_;
    SSL* ssl_ = nullptr;

    static int alpn_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
                             const unsigned char* in, unsigned int inlen, void* arg);
};
