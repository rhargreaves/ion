#if defined(__linux__)
#include "epoll_poller.h"

#include <unistd.h>

#include "spdlog/spdlog.h"

namespace ion {

EPollPoller::EPollPoller() : events_buffer_(64) {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0) {
        spdlog::error("epoll_create1: failed: {}", strerror(errno));
    }
}

EPollPoller::~EPollPoller() {
    if (epoll_fd_ >= 0) {
        close(epoll_fd_);
    }
}

uint32_t EPollPoller::to_epoll_events(PollEventType events) {
    uint32_t result = 0;
    if (has_event(events, PollEventType::Read)) {
        result |= EPOLLIN;
    }
    if (has_event(events, PollEventType::Write)) {
        result |= EPOLLOUT;
    }
    return result;
}

PollEventType EPollPoller::from_epoll_events(uint32_t events) {
    auto result = PollEventType::None;
    if (events & EPOLLIN) {
        result |= PollEventType::Read;
    }
    if (events & EPOLLOUT) {
        result |= PollEventType::Write;
    }
    if (events & EPOLLHUP) {
        result |= PollEventType::Hangup;
    }
    if (events & EPOLLERR) {
        result |= PollEventType::Error;
    }
    return result;
}

void EPollPoller::set(int fd, PollEventType event_types) {
    epoll_event ev{};
    ev.events = to_epoll_events(event_types);
    ev.data.fd = fd;

    const int op = registered_fds_.contains(fd) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    if (epoll_ctl(epoll_fd_, op, fd, &ev) < 0) {
        spdlog::warn("epoll_ctl: failed (fd: {}): {}", fd, strerror(errno));
        return;
    }
    registered_fds_[fd] = event_types;
}

void EPollPoller::remove(int fd) {
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        spdlog::warn("epoll_ctl: DEL failed (fd: {}): {}", fd, strerror(errno));
    }
    registered_fds_.erase(fd);
}

std::expected<std::vector<PollEvent>, PollError> EPollPoller::poll(
    std::chrono::milliseconds timeout) {
    const int num_events =
        epoll_wait(epoll_fd_, events_buffer_.data(), static_cast<int>(events_buffer_.size()),
                   static_cast<int>(timeout.count()));

    if (num_events == 0) {
        return std::unexpected{PollError::Timeout};
    }
    if (num_events < 0) {
        if (errno == EINTR) {
            return std::unexpected{PollError::InterruptedBySignal};
        }
        return std::unexpected{PollError::Error};
    }

    std::vector<PollEvent> result;
    result.reserve(num_events);
    for (int i = 0; i < num_events; ++i) {
        result.push_back({events_buffer_[i].data.fd, from_epoll_events(events_buffer_[i].events)});
    }

    // double buffer size if we hit the limit
    if (num_events == static_cast<int>(events_buffer_.size())) {
        events_buffer_.resize(events_buffer_.size() * 2);
    }

    return result;
}

}  // namespace ion
#endif
