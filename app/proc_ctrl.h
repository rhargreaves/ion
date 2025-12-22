

#pragma once

class ProcessControl {
   public:
    [[nodiscard]] static bool enable_no_new_privs();
    static bool check_no_new_privs();
};
