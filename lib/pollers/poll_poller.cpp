#include "poll_poller.h"

#include <poll.h>

#include "spdlog/spdlog.h"

namespace ion {

short PollPoller::event_from_poll_event(const PollEventType& events) {
    short result = 0;
    if (has_event(events, PollEventType::Read)) {
        result |= POLLIN;
    }
    if (has_event(events, PollEventType::Write)) {
        result |= POLLOUT;
    }
    return result;
}

PollEventType PollPoller::event_to_poll_event(short events) {
    auto result = PollEventType{};
    if (events & POLLIN) {
        result |= PollEventType::Read;
    }
    if (events & POLLOUT) {
        result |= PollEventType::Write;
    }
    if (events & POLLHUP) {
        result |= PollEventType::Hangup;
    }
    if (events & POLLERR) {
        result |= PollEventType::Error;
    }
    return result;
}

std::expected<std::vector<PollEvent>, PollError> PollPoller::poll(
    std::chrono::milliseconds timeout) {
    std::vector<pollfd> poll_fds{};
    poll_fds.reserve(fd_events_.size());

    for (const auto& [fd, event_types] : fd_events_) {
        poll_fds.push_back({fd, event_from_poll_event(event_types), 0});
    }

    const int ret = ::poll(poll_fds.data(), poll_fds.size(), static_cast<int>(timeout.count()));
    if (ret == 0) {
        spdlog::trace("poll timed out");
        return std::unexpected{PollError::Timeout};
    }
    if (ret < 0) {
        if (errno == EINTR) {
            spdlog::warn("poll interrupted by signal");
            return std::unexpected{PollError::InterruptedBySignal};
        }
        spdlog::error("poll failed: {}", strerror(errno));
        return std::unexpected{PollError::Error};
    }

    std::vector<PollEvent> result{};
    result.reserve(poll_fds.size());
    for (const auto& pfd : poll_fds) {
        result.push_back({pfd.fd, event_to_poll_event(pfd.revents)});
    }
    return result;
}

void PollPoller::set(int fd, PollEventType event_types) {
    fd_events_[fd] = event_types;
}

void PollPoller::remove(int fd) {
    fd_events_.erase(fd);
}

}  // namespace ion
