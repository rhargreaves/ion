#pragma once
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <thread>

static constexpr uint16_t MAX_PORT_WAIT_ATTEMPTS = 50;

bool wait_for_port(uint16_t port) {
    for (int i = 0; i < MAX_PORT_WAIT_ATTEMPTS; i++) {
        const int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
            continue;
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        const bool connected =
            connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0;
        close(sock);

        if (connected)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}
