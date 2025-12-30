#include "tls_context.h"

#include <spdlog/spdlog.h>

#include <array>
#include <fstream>

namespace ion {

TlsContext::TlsContext(const std::filesystem::path& cert_path,
                       const std::filesystem::path& key_path) {
    if (!exists(cert_path)) {
        throw std::runtime_error("certificate file not found");
    }

    if (!exists(key_path)) {
        throw std::runtime_error("private key file not found");
    }

    ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ctx_) {
        throw std::runtime_error("failed to create SSL context");
    }

    if (SSL_CTX_use_certificate_file(ctx_, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx_);
        throw std::runtime_error("failed to load certificate file");
    }

    if (SSL_CTX_use_PrivateKey_file(ctx_, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx_);
        throw std::runtime_error("failed to load private key file");
    }

    if (!SSL_CTX_check_private_key(ctx_)) {
        SSL_CTX_free(ctx_);
        throw std::runtime_error("private key does not match certificate");
    }

    SSL_CTX_set_alpn_select_cb(ctx_, alpn_callback, nullptr);

    SSL_CTX_set_keylog_callback(ctx_, [](const SSL*, const char* line) {
        if (const auto keylog_file = std::getenv("SSLKEYLOGFILE")) {
            std::ofstream file(keylog_file, std::ios::app);
            if (file.is_open()) {
                file << line << '\n';
            }
        }
    });
}

TlsContext::~TlsContext() {
    if (ctx_) {
        SSL_CTX_free(ctx_);
    }
}

SSL* TlsContext::create_ssl() const {
    SSL* ssl = SSL_new(ctx_);
    if (!ssl) {
        throw std::runtime_error("failed to create SSL object");
    }
    return ssl;
}

// ReSharper disable once CppDFAConstantFunctionResult
int TlsContext::alpn_callback(SSL*, const unsigned char** out, unsigned char* outlen,
                              const unsigned char* in, unsigned int inlen, void*) {
    static constexpr std::array<unsigned char, 3> supported_protos{'\x02', 'h', '2'};

    const int result =
        SSL_select_next_proto(const_cast<unsigned char**>(out), outlen, supported_protos.data(),
                              supported_protos.size(), in, inlen);
    if (result == OPENSSL_NPN_NEGOTIATED) {
        spdlog::debug("ALPN negotiated: {}",
                      std::string(reinterpret_cast<const char*>(*out), *outlen));
        return SSL_TLSEXT_ERR_OK;
    }

    *outlen = supported_protos[0];
    *out = supported_protos.data() + 1;
    spdlog::debug("ALPN negotiation failed, using default: {}",
                  std::string(reinterpret_cast<const char*>(*out), *outlen));
    return SSL_TLSEXT_ERR_OK;
}

}  // namespace ion
