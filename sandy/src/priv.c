#define _GNU_SOURCE
#include <stdio.h>

#include <unistd.h>
#include <sys/prctl.h>
#include <linux/close_range.h> 

int drop_privileges(void) {
    close_range(3U, ~0U, 0);
    if(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) {
        perror("PR_SET_NO_NEW_PRIVS");
        return -1;
    }
    if(prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0, 0, 0) < 0) {
        perror("PR_CAP_AMBIENT");
        return -1;
    }
    return 0;
}
