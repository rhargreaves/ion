#pragma once
#include <chrono>
#include <expected>
#include <vector>

namespace ion {

enum class PollError { Error, InterruptedBySignal, Timeout };

enum class PollEventType : int { None = 0, Read = 1, Write = 2, Hangup = 4, Error = 8 };

struct PollEvent {
    int fd;
    PollEventType events;
};

inline PollEventType operator&(PollEventType a, PollEventType b) {
    return static_cast<PollEventType>(std::to_underlying(a) & std::to_underlying(b));
}

inline PollEventType operator|(PollEventType a, PollEventType b) {
    return static_cast<PollEventType>(std::to_underlying(a) | std::to_underlying(b));
}

inline PollEventType& operator|=(PollEventType& lhs, PollEventType rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline bool has_event(PollEventType events, PollEventType check) {
    return static_cast<int>(events & check) != 0;
}

class Poller {
   public:
    virtual ~Poller() = default;
    virtual void set(int fd, PollEventType events) = 0;
    virtual void remove(int fd) = 0;
    virtual std::expected<std::vector<PollEvent>, PollError> poll(
        std::chrono::milliseconds timeout) = 0;

    static std::unique_ptr<Poller> create();
};

}  // namespace ion
