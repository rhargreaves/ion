

#pragma once

class ProcessControl {
   public:
    static void enable_no_new_privs();
    static bool check_no_new_privs();
};
