#include "proc_ctrl.h"

#ifdef __linux__
#include <sys/prctl.h>
#endif

bool ProcessControl::enable_no_new_privs() {
    bool no_new_privs = false;
#ifdef __linux__
    int result = prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0);
    if (result == 1)
        no_new_privs = true;
#endif
    return no_new_privs;
}

bool ProcessControl::check_no_new_privs() {
    bool enabled = false;
#ifdef __linux__
    enabled = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0;
#endif
    return enabled;
}
