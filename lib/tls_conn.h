#pragma once
#include <openssl/ssl.h>
#include <unistd.h>

#include <expected>
#include <filesystem>
#include <span>

#include "socket_fd.h"

namespace ion {

enum class TransportError {
    WantReadOrWrite,
    WriteError,
    ConnectionClosed,
    ProtocolError,
    OtherError
};
enum class ClientIpError { GetPeerNameFailed, UnknownIpFormat };

class TlsConnection {
   public:
    explicit TlsConnection(SocketFd&& client_fd, const std::filesystem::path& cert_path,
                           const std::filesystem::path& key_path);
    ~TlsConnection();

    TlsConnection(TlsConnection&& other) noexcept;
    TlsConnection& operator=(TlsConnection&& other) noexcept;

    TlsConnection(const TlsConnection&) = delete;
    TlsConnection& operator=(const TlsConnection&) = delete;

    static void print_debug_to_stderr();
    std::expected<ssize_t, TransportError> read(std::span<uint8_t> buffer) const;
    std::expected<ssize_t, TransportError> write(std::span<const uint8_t> buffer) const;
    [[nodiscard]] bool has_data() const;
    [[nodiscard]] bool handshake() const;
    void graceful_shutdown() const;
    std::expected<std::string, ClientIpError> client_ip() const;

   private:
    SocketFd client_fd_;
    SSL* ssl_ = nullptr;

    static int alpn_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
                             const unsigned char* in, unsigned int inlen, void* arg);
};

}  // namespace ion
