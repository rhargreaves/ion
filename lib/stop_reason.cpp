

#include "stop_reason.h"

#include <string_view>

namespace ion {

std::string_view StopReasonHelper::to_string(const StopReason& reason) {
    switch (reason) {
        case StopReason::Signal:
            return "signal";
        case StopReason::StdOutPipeBroken:
            return "stdout pipe broken";
        case StopReason::StdErrPipeBroken:
            return "stderr pipe broken";
        default:
            return "other";
    }
}

}  // namespace ion
