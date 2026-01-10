#pragma once
#include <string>

namespace ion::app {

class Telemetry {
   public:
    explicit Telemetry(const std::string& service_name);
    ~Telemetry();

    Telemetry(const Telemetry&) = delete;
    Telemetry& operator=(const Telemetry&) = delete;
};

}  // namespace ion::app
