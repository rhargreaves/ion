#include "proc_ctrl.h"

#include <spdlog/spdlog.h>

#include <cerrno>

#ifdef __linux__
#include <sys/prctl.h>
#endif

void ProcessControl::enable_no_new_privs() {
#ifdef __linux__
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
        spdlog::warn("failed to set PR_SET_NO_NEW_PRIVS: {}", std::strerror(errno));
        return;
    }
    spdlog::debug("PR_SET_NO_NEW_PRIVS enabled");
#endif
}

bool ProcessControl::check_no_new_privs() {
#ifdef __linux__
    int result = prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0);
    if (result == -1) {
        spdlog::warn("failed to get PR_GET_NO_NEW_PRIVS: {}", std::strerror(errno));
        return false;
    }
    return result == 1;
#else
    return false;
#endif
}
