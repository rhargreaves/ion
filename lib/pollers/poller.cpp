#include "poller.h"

#include "poll_poller.h"

#if defined(__linux__)
#include "epoll_poller.h"
#endif

namespace ion {

std::unique_ptr<Poller> Poller::create() {
#if defined(__linux__)
    return std::make_unique<EPollPoller>();
#else
    return std::make_unique<PollPoller>();
#endif
}

}  // namespace ion
