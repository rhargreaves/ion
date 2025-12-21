#pragma once
#include <openssl/ssl.h>

#include <expected>
#include <filesystem>
#include <span>

#include "socket_fd.h"
#include "transport.h"

namespace ion {

class TlsTransport : public Transport {
   public:
    explicit TlsTransport(SocketFd&& client_fd, const std::filesystem::path& cert_path,
                          const std::filesystem::path& key_path);
    ~TlsTransport() override;

    TlsTransport(TlsTransport&& other) noexcept;
    TlsTransport& operator=(TlsTransport&& other) noexcept;

    TlsTransport(const TlsTransport&) = delete;
    TlsTransport& operator=(const TlsTransport&) = delete;

    static void print_debug_to_stderr();
    std::expected<ssize_t, TransportError> read(std::span<uint8_t> buffer) const override;
    std::expected<ssize_t, TransportError> write(std::span<const uint8_t> buffer) const override;
    [[nodiscard]] bool has_data() const override;
    [[nodiscard]] bool handshake() const;
    void graceful_shutdown() const override;
    std::expected<std::string, ClientIpError> client_ip() const override;

   private:
    SocketFd client_fd_;
    SSL* ssl_ = nullptr;

    static int alpn_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
                             const unsigned char* in, unsigned int inlen, void* arg);
};

}  // namespace ion
