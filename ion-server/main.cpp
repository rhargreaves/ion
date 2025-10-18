#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

static constexpr uint16_t s_port = 8443;

int alpn_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
                  const unsigned char* in, unsigned int inlen, void* arg) {
    // Define the protocols we support (in order of preference)
    static const unsigned char supported_protos[] = "\x02h2\x08http/1.1";
    static const unsigned int supported_protos_len = sizeof(supported_protos) - 1;

    // Select a protocol from the client's list
    int result = SSL_select_next_proto(
        const_cast<unsigned char**>(out), outlen,
        supported_protos, supported_protos_len,
        in, inlen
    );

    if (result == OPENSSL_NPN_NEGOTIATED) {
        std::cout << "ALPN negotiated: ";
        std::cout.write(reinterpret_cast<const char*>(*out), *outlen) << std::endl;
        return SSL_TLSEXT_ERR_OK;
    }

    // No match found - use the first protocol we support as fallback
    *out = supported_protos + 1;  // Skip the length byte
    *outlen = supported_protos[0]; // Get the length
    std::cout << "ALPN negotiation failed, using default: ";
    std::cout.write(reinterpret_cast<const char*>(*out), *outlen) << std::endl;

    return SSL_TLSEXT_ERR_OK;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(s_port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "[ion] Listening on port " << s_port << "..." << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected." << std::endl;

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        std::cerr << "Error: Failed to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error: Failed to load certificate file" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error: Failed to load private key file" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        std::cerr << "Error: Private key does not match certificate" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    SSL_CTX_set_alpn_select_cb(ctx, alpn_callback, NULL);

    SSL* ssl = SSL_new(ctx);
    if (!ssl) {
        std::cerr << "Error: Failed to create SSL object" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    if (SSL_set_fd(ssl, client_fd) != 1) {
        std::cerr << "Error: Failed to set SSL file descriptor" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    if (SSL_accept(ssl) <= 0) {
        std::cerr << "Error: SSL handshake failed" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    std::cout << "SSL handshake completed successfully." << std::endl;

    const std::string msg { "Hello TCP World\n" };
    ssize_t sent = SSL_write(ssl, msg.data(), msg.length());
    if (sent < 0) {
        perror("write");
    }

    close(client_fd);
    close(server_fd);
    std::cout << "Connection closed." << std::endl;

    return 0;
}