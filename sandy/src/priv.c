#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/prctl.h>
#include <linux/capability.h>
#include <linux/close_range.h> 

int drop_privileges(void) {
    // make sure everything but std io fds are close when exec.
    close_range(3U, ~0U, CLOSE_RANGE_CLOEXEC);
    if(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) {
        perror("PR_SET_NO_NEW_PRIVS");
        return -1;
    }
    if(prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0, 0, 0) < 0) {
        perror("PR_CAP_AMBIENT");
        return -1;
    }
    for (int cap = 0; cap <= CAP_LAST_CAP; cap++) {
        if (prctl(PR_CAPBSET_DROP, cap, 0, 0, 0) < 0) {
            // I am just blindly looping through so if one is invalid
            // just continue.
            if(errno == EINVAL) {
                continue;
            }
            perror("PR_CAPBSET_DROP");
            return -1;
        }
    }
    return 0;
}
