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
        "  --help                   Show this help\n"
        "  --bindf    HOST TARGET   Bind HOST file as read only at TARGET\n"
        "  --rw-bindf HOST TARGET   Bind HOST file as read & write at TARGET\n",
        prog,
        prog
    );
}

typedef struct {
    const char *host;
    const char *target;
    bool read_only;
} BindInfo;
#define MAX_FBIND 64

int main(int argc, char **argv) {
    BindInfo fbind[MAX_FBIND] = { 0 };
    bool args   = false;
    int fbinds  = 0;
    int cmd_idx = -1;

    for (int i = 1; i < argc; i++) {
        char* cur = argv[i];
        if(strcmp(cur, "--help") == 0 || strcmp(cur, "-h") == 0) {
            usage(argv[0]);
            return 0;
        }
        if(strcmp(cur, "--") == 0) {
            cmd_idx = i + 1;
            break;
        }

        bool bindf = strcmp(argv[i], "--bindf") == 0;
        bool bindf_rw = (strcmp(argv[i], "--rw-bindf") == 0);
        if(bindf || bindf_rw) {
            if (i + 2 >= argc) {
                fprintf(stderr, "missing arguments for %s\n", argv[i]);
                return 1;
            }
            if (fbinds >= MAX_FBIND) {
                fprintf(stderr, "too many file binds\n");
                return 1;
            }
            fbind[fbinds].host      = argv[i + 1];
            fbind[fbinds].target    = argv[i + 2];
            fbind[fbinds].read_only = bindf;
            fbinds++;
            i += 2;
            args = true;
        }

        // If something was reconized as a flag/arg
        // the only way to break is with `--` or reaching the end.
        if(!args) { 
            cmd_idx = i;
            break;
        }
    }
    if(cmd_idx < 0 || cmd_idx >= argc) {
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