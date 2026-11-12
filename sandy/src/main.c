#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mount.h"
#include "priv.h"


static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [OPTIONS] -- COMMAND [ARGS...]\n"
        "       %s COMMAND [ARGS...]\n"
        "Minimal process sandbox. I wrote this more for fun, not because\n"
        "I needed since there otherw ways I could achieve similar.\n"
        "Options (before --):\n"
        "  --help                  Show this help\n",
        prog,
        prog
    );
}

int main(int argc, char **argv) {
    int cmd_idx = -1;

    for (int i = 1; i < argc; i++) {
        char* cur = argv[i];
        if(strcmp(cur, "--help") == 0 || strcmp(cur, "-h") == 0) {
            usage(argv[0]);
            return 0;
        }
    }


    if(cmd_idx < 0) {
        fprintf(stderr, "%s: Error: No command to run\n", argv[0]);  
        usage(argv[0]);
        return -1;
    }


    int mnt = setup_mount();
    if(mnt < 0) {
        fprintf(stderr, "Failed to create mount point");
        return -1;
    }
    if(setup_devices(mnt) < 0) {
        fprintf(stderr, "Failed to setup devices");
        close(mnt);
        return -1;
    }
    drop_privileges();

    // Todo process args to start a process with some `exec{?}` call
    return 0;
}