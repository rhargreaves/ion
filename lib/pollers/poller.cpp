#include "poller.h"

#include "poll_poller.h"

namespace ion {

std::unique_ptr<Poller> Poller::create() {
    return std::make_unique<PollPoller>();
}

}  // namespace ion
