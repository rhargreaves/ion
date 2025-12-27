#pragma once
#include <unordered_map>
#include <vector>

#include "poller.h"

namespace ion {

class PollPoller : public Poller {
   public:
    PollPoller() = default;
    ~PollPoller() override = default;

    std::vector<PollEvent> poll(std::chrono::milliseconds timeout) override;
    void set(int fd, PollEventType event_types) override;
    void remove(int fd) override;

   private:
    std::unordered_map<int, PollEventType> fd_events_{};

    static short event_from_poll_event(const PollEventType& event);
    static PollEventType event_to_poll_event(short events);
};

}  // namespace ion
