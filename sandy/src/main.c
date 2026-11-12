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
        "  --bindf    HOST TARGET   Bind HOST file as read only at TARGET\n"
        "  --rw-bindf HOST TARGET   Bind HOST file as read & write at TARGET\n"
        "  --bindd    HOST TARGET   Bind HOST directory as read only at TARGET\n"
        "  --rw-bindd HOST TARGET   Bind HOST directory as read & write at TARGET\n"
        "  --help                   Show this help\n",
        prog,
        prog
    );
}

typedef struct {
    const char *host;
    const char *target;
    bool read_only;
} BindInfo;
#define MAX_BIND 64

int main(int argc, char **argv) {
    BindInfo bindf[MAX_BIND] = { 0 };
    BindInfo bindd[MAX_BIND] = { 0 };
    bool args   = false;
    int bindfs  = 0;
    int bindds  = 0;
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

        bool bindf_ro = strcmp(argv[i], "--bindf") == 0;
        bool bindf_rw = (strcmp(argv[i], "--rw-bindf") == 0);
        if(bindf_ro || bindf_rw) {
            if (i + 2 >= argc) {
                fprintf(stderr, "missing arguments for %s\n", argv[i]);
                return 1;
            }
            if (bindfs >= MAX_BIND) {
                fprintf(stderr, "too many file binds\n");
                return 1;
            }
            bindf[bindfs].host      = argv[i + 1];
            bindf[bindfs].target    = argv[i + 2];
            bindf[bindfs].read_only = bindf_ro;
            bindfs++;
            i += 2;
            args = true;
            continue;
        }

        bool bindd_ro = strcmp(argv[i], "--bindd") == 0;
        bool bindd_rw = (strcmp(argv[i], "--rw-bindd") == 0);
        if(bindd_ro || bindd_rw) {
            if (i + 2 >= argc) {
                fprintf(stderr, "missing arguments for %s\n", argv[i]);
                return 1;
            }
            if (bindds >= MAX_BIND) {
                fprintf(stderr, "too many file binds\n");
                return 1;
            }
            bindd[bindds].host      = argv[i + 1];
            bindd[bindds].target    = argv[i + 2];
            bindd[bindds].read_only = bindd_ro;
            bindds++;
            i += 2;
            args = true;
            continue;
        }

        // If something was recognized as a flag/arg
        // the only way to break is with `--` or reaching the end.
        if(!args) { 
            cmd_idx = i;
            break;
        }
    }
    if(cmd_idx < 0 || cmd_idx >= argc) {
        fprintf(stderr, "%s: Error: No command to run\n", argv[0]);  
        usage(argv[0]);
        return 1;
    }


    int mnt = setup_mount();
    if(mnt < 0) {
        fprintf(stderr, "Failed to create mount point");
        return 1;
    }
    if(setup_devices(mnt) < 0) {
        fprintf(stderr, "Failed to setup devices");
        close(mnt);
        return 1;
    }

    // User directory binds
    for (int i = 0; i < bindds; i++) {
        int rc = bind_dir(mnt, bindd[i].host, bindd[i].target, bindd[i].read_only);
        if (rc < 0) {
            fprintf(stderr, "Failed to bind directory '%s' -> '%s'\n", bindd[i].host, bindd[i].target);
            close(mnt);
            return 1;
        }
    }

    // User file binds
    for (int i = 0; i < bindfs; i++) {
        int rc = bind_file(mnt, bindf[i].host, bindf[i].target, bindf[i].read_only);
        if (rc < 0) {
            fprintf(stderr, "Failed to bind file '%s' -> '%s'\n", bindf[i].host, bindf[i].target);
            close(mnt);
            return 1;
        }
    }

    if(swap_root_to_mount(mnt) < 0) {
        fprintf(stderr, "Swapping root failed");
        return 1;
    }

    if(drop_privileges() < 0) {
        fprintf(stderr, "Dropping privileges failed");
        return 1;
    }

    // Todo process args to start a process with some `exec{?}` call
    return 0;
}