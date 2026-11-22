#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "conf.h"
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
        "  --bindf      TARGET HOST  Bind HOST file as read only at TARGET\n"
        "  --bindf-rw   TARGET HOST  Bind HOST file as read & write at TARGET\n"
        "  --bindd      TARGET HOST  Bind HOST directory as read only at TARGET\n"
        "  --bindd-rw   TARGET HOST  Bind HOST directory as read & write at TARGET\n"
        "  --conf       FILE         Load settings such as binds from a file\n"
        "  --exec-host               COMMAND is started from host path\n"
        "  --bind-cmd                Bind COMMAND in the environment as written.\n"
        "                            If the path is relative the environment root is\n"
        "                            used as the CWD on where to bind in the environment.\n"
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
    BindInfo*   dirs;
    size_t      dirs_cap;
    size_t      dirs_cnt;
    BindInfo*   files;
    size_t      files_cnt;
    size_t      files_cap;
    size_t      cmd_idx;
    const char* conf_file;
    bool        bind_cmd;
    bool        exec_host;
    bool        help;
    bool        verbose;
    bool        color;
} Config;

bool process_args(int argc, char** argv, Config* conf) {
    const char* cmd = "sandy";
    if(argc >= 1) {
        cmd = argv[0];
    }
    // Not enough args.
    if(argc < 2) {
        usage(cmd);
        return false;
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

        if(strcmp(cur, "--conf") == 0) {
                if((i + 1) >= sep) {
                fprintf(stderr, "Error: missing file path for `--conf`\n");
                return false;
            }
            conf->conf_file = argv[i + 1];
            i += 1;
            continue;
        }

        bool bindd_ro = strcmp(argv[i], "--bindd") == 0;
        bool bindd_rw = strcmp(argv[i], "--bindd-rw") == 0;
        if(bindd_ro || bindd_rw) {
            if((i + 2) >= sep) {
                fprintf(stderr, "Error: missing arguments for `%s`\n", argv[i]);
                return false;
            }
            size_t cnt = conf->dirs_cnt;
            if(cnt >= conf->dirs_cap) {
                fprintf(stderr, "Error: too many directory binds, max: %zu\n", conf->dirs_cap);
                return false;
            }
            conf->dirs[cnt].target    = argv[i + 1];
            conf->dirs[cnt].host      = argv[i + 2];
            conf->dirs[cnt].read_only = bindd_ro;
            conf->dirs_cnt = cnt + 1;
            i += 2;
            continue;
        }

        bool bindf_ro = strcmp(argv[i], "--bindf") == 0;
        bool bindf_rw = strcmp(argv[i], "--bindf-rw") == 0;
        if(bindf_ro || bindf_rw) {
            if((i + 2) >= sep) {
                fprintf(stderr, "Error: missing arguments for `%s`\n", argv[i]);
                return false;
            }
            size_t cnt = conf->files_cnt;
            if(cnt >= conf->files_cap) {
                fprintf(stderr, "Error: too many file binds, max: %zu\n", conf->files_cap);
                return false;
            }
            conf->files[cnt].target    = argv[i + 1];
            conf->files[cnt].host      = argv[i + 2];
            conf->files[cnt].read_only = bindf_ro;
            conf->files_cnt = cnt + 1;
            i += 2;
            continue;
        }

        fprintf(stderr, "%s: error: unrecognized command-line option ‘%s’\n\n", cmd, cur);
        usage(cmd);
        return false;
    }
    // No flags/args cases
    if(sep < 0) {
        conf->bind_cmd = true;
        conf->cmd_idx = 1;
        return true;

    }
    // Make sure there is at least on arge after the --
    if((sep + 1) >= argc) {
        usage(cmd);
        return false;
    }
    conf->cmd_idx = sep + 1;
    return true;
}

MountErr process_cmd(ConFileCmd* cmd, PathCtx* ctx, Verbose v) {
    ctx->host_src = cmd->src;
    ctx->env_dst  = cmd->dst;
    MountErr   me = M_SUCCESS;
    switch(cmd->kind) {
        case CF_K_LINK:
            me = make_symlink(ctx, v);
            verbose_mnt_action(ctx, v, me, "link");
            break;
        case CF_K_COPY:
            me = copy_file(ctx, v, cmd->mode);
            verbose_mnt_action(ctx, v, me, "copy");
            break;
        case CF_K_BINDF:
            me = bind_file(ctx, v, true);
            verbose_mnt_action(ctx, v, me, "mount");
            break;
        case CF_K_BINDF_RW:
            me = bind_file(ctx, v, false);
            verbose_mnt_action(ctx, v, me, "mount");
            break;
        case CF_K_BINDD:
            me = bind_dir(ctx, v, true);
            verbose_mnt_action(ctx, v, me, "mount");
            break;
        case CF_K_BINDD_RW:
            me = bind_dir(ctx, v, false);
            verbose_mnt_action(ctx, v, me, "mount");
            break;
        default:
            break;
    }
    return me;
}

bool process_conf(const char* path, PathCtx* ctx, Verbose v) {
    ConFile* cf  = nullptr;
    verbose_header(v, "Loading Config File");

    ConFileErr e = con_file_new(&cf, path);
    verbose_line(v, "open", path, e != CF_SUCCESS);
    if(e != CF_SUCCESS) {
        con_file_print_new_error(e, path);
        return false;
    }

    MountErr   me = M_SUCCESS;
    ConFileCmd cmd = { 0 };
    for(;;) {
        e = con_file_next_cmd(cf, &cmd);
        if(e == CF_CMD_DONE) {
            e = CF_SUCCESS;
            break;
        }
        if(e == CF_CMD_EMPTY) {
            e = CF_SUCCESS;
            continue;
        }
        if(e != CF_SUCCESS) {
            break;
        }
        me = process_cmd(&cmd, ctx, v);
        if(me != M_SUCCESS) {
            break;
        }
    }

    con_file_free(cf);
    if(me != M_SUCCESS) {
        print_mount_error(ctx, me);
        return false;
    }
    
    if(e != CF_SUCCESS) {
        con_file_print_cmd_error(e, path, con_file_line_num(cf));
        return false;
    }
    return true;
}

int open_cmd(const char* path, Verbose v) {
    verbose_header(v, "Opening Command");
    int cmd = open(path, O_RDONLY | O_CLOEXEC);
    int errnum = errno;
    verbose_line(v, "open", path, cmd < 0);
    if(cmd >= 0) {
        return cmd;
    }
    switch(errnum) {
        case ENOENT:
            fprintf(stderr, "Error: Command not found: '%s'\n", path);
            break;
        case ENAMETOOLONG:
            fprintf(stderr, "Error: Command path name is too long: '%s'\n", path);
            break;
        case EACCES:
        case EPERM:
            fprintf(stderr, "Error: Permission denied for command: '%s'\n", path);
            break;
        case ELOOP:
        case ENOTDIR:
            fprintf(stderr, "Error: A path component of the command is not walkable: '%s'\n", path);
            break;
        default:
            D_ERROR("A system failure occurred when opening command", errnum);
            break;
    }
    return -1;
}

int main(int argc, char **argv) {
    BindInfo bindf[MAX_BIND] = { 0 };
    BindInfo bindd[MAX_BIND] = { 0 };
    Config   conf = {
        .dirs      = bindd,
        .dirs_cap  = MAX_BIND,
        .files     = bindf,
        .files_cap = MAX_BIND,
        .cmd_idx   = -1,
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

    // Fork so we are in the new PID name space.
    pid_t pid = fork();
    if(pid < 0) {
        D_ERROR("fork() failed", errno);
        close(mnt);
        return 1;
    }
    if(pid > 0) {
        close(mnt);
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            D_ERROR("waitpid() failed", errno);
            return 1;
        }
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    
    if(!setup_proc(mnt, v)) {
        close(mnt);
        return 1;
    }

    //if(!setup_libs(mnt, v)) {
    //    close(mnt);
    //    return 1;
    //}

    if(!setup_devices(mnt, v)) {
        close(mnt);
        return 1;
    }

    uint8_t buff[PATH_MAX + 1];
    PathCtx ctx = {
        .mount_fd  = mnt,
        .buff      = buff,
        .buff_cap  = PATH_MAX + 1
    };

    // User directory binds
    if(conf.dirs_cnt > 0) {
        verbose_header(v, "Mounting User Directories");
        for (size_t i = 0; i < conf.dirs_cnt; i++) {
            ctx.host_src = (const uint8_t*)conf.dirs[i].host;
            ctx.env_dst  = (const uint8_t*)conf.dirs[i].target;

            MountErr e = bind_dir(&ctx, v, conf.dirs[i].read_only);
            verbose_mnt_action(&ctx, v, e, "mount");
            if(e != M_SUCCESS) {
                close(mnt);
                print_mount_error(&ctx, e);
                return 1;
            }
        }
    }

    if(conf.files_cnt > 0) {
        verbose_header(v, "Mounting User Files");
        for (size_t i = 0; i < conf.files_cnt; i++) {
            ctx.host_src = (const uint8_t*)conf.files[i].host;
            ctx.env_dst  = (const uint8_t*)conf.files[i].target;

            MountErr e = bind_file(&ctx, v, conf.files[i].read_only);
            verbose_mnt_action(&ctx, v, e, "mount");
            if(e != M_SUCCESS) {
                close(mnt);
                print_mount_error(&ctx, e);
                return 1;
            }
        }
    }

    if(conf.conf_file != nullptr) {
        if(!process_conf(conf.conf_file, &ctx, v)) {
            return 1;
        }
    }

    if(conf.bind_cmd) {
        verbose_header(v, "Binding Command");
        char* cmd = argv[conf.cmd_idx];
        ctx.host_src = (const uint8_t*)cmd;
        ctx.env_dst  = (const uint8_t*)cmd;
        MountErr e = bind_file(&ctx, v, false);
        verbose_mnt_action(&ctx, v, e, "mount");
        if(e != M_SUCCESS) {
            close(mnt);
            print_mount_error(&ctx, e);
            return 1;
        }
    }

    int cmd_fd = -1;
    // Before swapping root get the cmd fd
    if(conf.exec_host) {
        cmd_fd = open_cmd(argv[conf.cmd_idx], v);
        if(cmd_fd < 0) {
            return 1;
        }
    }

    if(!swap_root_to_mount(mnt, v)) {
        return 1;
    }

    if(drop_privileges() < 0) {
        D_ERROR("Dropping privileges failed");
        return 1;
    }

    if(cmd_fd < 0) {
        cmd_fd = open_cmd(argv[conf.cmd_idx], v);
        if(cmd_fd < 0) {
            return 1;
        }
    }

    char *const env_envp[] = { NULL };
    int res = execveat(cmd_fd, "", &(argv[conf.cmd_idx]), env_envp, AT_EMPTY_PATH);
    if(res < 0) {
        perror("execveat");
        return 1;
    }
    return 0;
}
