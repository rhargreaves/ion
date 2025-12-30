#pragma once

#include <openssl/ssl.h>

#include <filesystem>

namespace ion {

class TlsContext {
   public:
    TlsContext(const std::filesystem::path& cert_path, const std::filesystem::path& key_path);
    ~TlsContext();
    [[nodiscard]] SSL* create_ssl() const;

    TlsContext(const TlsContext&) = delete;
    TlsContext& operator=(const TlsContext&) = delete;
    TlsContext(TlsContext&& other) noexcept = delete;
    TlsContext& operator=(TlsContext&& other) noexcept = delete;

   private:
    SSL_CTX* ctx_{};

    static int alpn_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
                             const unsigned char* in, unsigned int inlen, void* arg);
};

}  // namespace ion
