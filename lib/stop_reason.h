

#pragma once
#include <string_view>

namespace ion {

enum class StopReason { Other, Signal, StdOutPipeBroken, StdErrPipeBroken };

class StopReasonHelper {
   public:
    static std::string_view to_string(const StopReason& reason);
};

}  // namespace ion
