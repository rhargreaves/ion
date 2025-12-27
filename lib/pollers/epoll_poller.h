#pragma once
#if defined(__linux__)
#include <sys/epoll.h>

#include <expected>
#include <unordered_map>
#include <vector>

#include "poller.h"

namespace ion {

class EPollPoller : public Poller {
   public:
    EPollPoller();
    ~EPollPoller() override;

    std::expected<std::vector<PollEvent>, PollError> poll(
        std::chrono::milliseconds timeout) override;
    void set(int fd, PollEventType event_types) override;
    void remove(int fd) override;

   private:
    int epoll_fd_{-1};
    std::unordered_map<int, PollEventType> registered_fds_{};
    std::vector<epoll_event> events_buffer_;

    static uint32_t to_epoll_events(PollEventType events);
    static PollEventType from_epoll_events(uint32_t events);
};

}  // namespace ion
#endif
