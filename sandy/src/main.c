#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "diag.h"
#include "mount.h"
#include "priv.h"
#include "verbose.h"


static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [OPTIONS] -- COMMAND [ARGS...]\n"
        "       %s COMMAND [ARGS...] (implies --cmd-bind)\n"
        "A Minimal process sandbox. COMMAND expects a path in sandbox environment\n"
        "unless `--exec-host` is passed to `%s` to run. I wrote this more for\n"
        "fun, and not because I needed to since there other ways I could achieve\n"
        "similar.\n\n"
        "Options (before --):\n"
        "  --bindf      HOST TARGET  Bind HOST file as read only at TARGET\n"
        "  --bindf-rw   HOST TARGET  Bind HOST file as read & write at TARGET\n"
        "  --bindd      HOST TARGET  Bind HOST directory as read only at TARGET\n"
        "  --bindd-rw   HOST TARGET  Bind HOST directory as read & write at TARGET\n"
        "  --exec-host               COMMAND is started from host path\n"
        "  --bind-cmd                Bind COMMAND in the environment as written\n" 
        "  --verbose                 Print extra information as environment is setup\n"
        "  --color                   Use ANSI control sequences on stdio/stderr\n"
        "  --help                    Show this help\n",
        prog,
        prog,
        prog
    );
}

typedef struct bind_info_s {
    const char *host;
    const char *target;
    bool read_only;
} BindInfo;
#define MAX_BIND 64

typedef struct config_s {
    BindInfo* dirs;
    size_t    dirs_cap;
    size_t    dirs_cnt;
    BindInfo* files;
    size_t    files_cnt;
    size_t    files_cap;
    size_t    cmd_idx;
    bool      bind_cmd;
    bool      exec_host;
    bool      help;
    bool      verbose;
    bool      color;
} Config;

bool process_args(int argc, char** argv, Config* conf) {
    const char* cmd = "sandy";
    if(argc >= 1) {
        cmd = argv[0];
    }

    // Find `--` index
    int sep = -1;
    for(int i = 1; i < argc; i++) {
        char *a = argv[i];
        if(a[0] == '-' && a[1] == '-' && a[2] == '\0') {
            sep = i;
            break;
        }
    }
    // Process all args before `--`
    for(int i = 1; i < sep; i++) {
        char* cur = argv[i];
        if(strcmp(cur, "--help") == 0 || strcmp(cur, "-h") == 0) {
            usage(cmd);
            conf->help = true;
            return true;
        }

        if(strcmp(cur, "--bind-cmd") == 0) {
            conf->bind_cmd = true;
            continue;
        }

        if(strcmp(cur, "--exec-host") == 0) {
            conf->exec_host = true;
            continue;
        }

        if(strcmp(cur, "--verbose") == 0) {
            conf->verbose = true;
            continue;
        }

        if(strcmp(cur, "--color") == 0) {
            conf->color = true;
            continue;
        }

        bool bindd_ro = strcmp(argv[i], "--bindd") == 0;
        bool bindd_rw = strcmp(argv[i], "--bindd-rw") == 0;
        if(bindd_ro || bindd_rw) {
            if(i + 2 >= argc) {
                fprintf(stderr, "missing arguments for `%s`\n", argv[i]);
                return false;
            }
            size_t cnt = conf->dirs_cnt;
            if(cnt >= conf->dirs_cap) {
                fprintf(stderr, "too many directory binds, max: %zu\n", conf->dirs_cap);
                return false;
            }
            conf->dirs[cnt].host      = argv[i + 1];
            conf->dirs[cnt].target    = argv[i + 2];
            conf->dirs[cnt].read_only = bindd_ro;
            conf->dirs_cnt = cnt + 1;
            i += 2;
            continue;
        }

        bool bindf_ro = strcmp(argv[i], "--bindf") == 0;
        bool bindf_rw = strcmp(argv[i], "--bindf-rw") == 0;
        if(bindf_ro || bindf_rw) {
            if(i + 2 >= argc) {
                fprintf(stderr, "missing arguments for `%s`\n", argv[i]);
                return false;
            }
            size_t cnt = conf->files_cnt;
            if(cnt >= conf->files_cap) {
                fprintf(stderr, "too many file binds, max: %zu\n", conf->files_cap);
                return false;
            }
            conf->files[cnt].host      = argv[i + 1];
            conf->files[cnt].target    = argv[i + 2];
            conf->files[cnt].read_only = bindd_ro;
            conf->files_cnt = cnt + 1;
            i += 2;
            continue;
        }

        fprintf(stderr, "%s: error: unrecognized command-line option ‘%s’\n\n", cmd, cur);
        usage(cmd);
        return false;
    }


    return true;
}

int main(int argc, char **argv) {
    BindInfo bindf[MAX_BIND] = { 0 };
    BindInfo bindd[MAX_BIND] = { 0 };
    Config   conf = {
        .dirs      = bindd,
        .dirs_cap  = MAX_BIND,
        .files     = bindf,
        .files_cap = MAX_BIND,
    };
    if(!process_args(argc, argv, &conf)) {
        return 1;
    }
    if(conf.help) {
        return 0;
    }
    diag_init(DIAG_DEBUG, conf.color);
    Verbose v = verbose_init(conf.verbose, conf.color);

    int mnt = setup_mount(v);
    if(mnt < 0) {
        fwrite("Failed to create mount point\n", 1, 29, stderr);
        return 1;
    }

    //if(!setup_devices(mnt, v)) {
    //    close(mnt);
    //    return 1;
    //}

/*
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

    // Todo process args to start a process with some `exec{?}` call*/
    return 0;
}
