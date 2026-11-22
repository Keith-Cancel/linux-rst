#define _GNU_SOURCE
#include "diag.h"
#include "mount.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <linux/openat2.h>

static bool fd_is_dev      (int fd);
static bool fd_is_dir      (int fd);
static bool fd_is_file     (int fd);
static bool enter_namespace(void);

static MountErr open_host_tree       (PathCtx* ctx, unsigned flags, int* fd_out);
static MountErr open_host            (PathCtx* ctx, unsigned flags, int* fd_out);
static MountErr mkdir_env            (int mnt, const uint8_t* path, mode_t mode, Verbose v);
static MountErr mkfile_env           (int mnt, const uint8_t* path, mode_t mode, Verbose v, int* fd_out);
static MountErr mkdir_env_parent     (int mnt, uint8_t* buf, Verbose verbose);
static MountErr prep_and_check_paths (PathCtx* ctx);
static MountErr prep_env_path        (const uint8_t* path, uint8_t buff[], size_t blen);

static int pivot_root(const char* new_root, const char* put_old) {
    return syscall(SYS_pivot_root, new_root, put_old);
}

int setup_mount(Verbose v) {
    v = verbose_set_title_width(v, 10);
    verbose_header(v, "Configuring: tmpfs");
    if (!enter_namespace()) {
        return -1;
    }

    int fd  = fsopen("tmpfs", FSOPEN_CLOEXEC);
    int err = errno;
    verbose_line(v, "tmpfs", "fsopen()", fd < 0);
    if(fd < 0) {
        D_ERROR("Failed to open 'tmpfs' filesystem context via fsopen()", err);
        return -1;
    }

    int res = fsconfig(fd, FSCONFIG_SET_STRING, "size", "512M", 0);
    err     = errno;
    verbose_line(v, "tmpfs size", "512M", res < 0);
    if(res < 0) {
        D_ERROR("Failed to set tmpfs option 'size=512M' via fsconfig()", err);
        close(fd);
        return -1;
    }

    res = fsconfig(fd, FSCONFIG_SET_STRING, "mode", "0755", 0);
    err = errno;
    verbose_line(v, "tmpfs mode", "0755", res < 0);
    if(res < 0) {
        D_ERROR("Failed to set tmpfs option 'mode=0755' via fsconfig()", err);
        close(fd);
        return -1;
    }

    res = fsconfig(fd, FSCONFIG_CMD_CREATE_EXCL, NULL, NULL, 0);
    err = errno;
    verbose_line(v, "tmpfs", "exclusive", res < 0);
    if(res < 0) {
        D_ERROR("Failed to create exclusive tmpfs instance via fsconfig()", err);
        close(fd);
        return -1;
    }

    int mnt = fsmount(fd, FSMOUNT_CLOEXEC, MOUNT_ATTR_NOSUID | MOUNT_ATTR_NODEV);
    err     = errno;
    verbose_line(v, "tmpfs", "fsmount()", mnt < 0);
    close(fd);
    if(mnt < 0) {
        D_ERROR("Failed to create detached tmpfs mount", err);
        return -1;
    }
    return mnt;
}

bool setup_proc(int mnt, Verbose v) {
    v = verbose_set_title_width(v, 12);
    verbose_header(v, "Configuring: /proc");

    MountErr e = mkdir_env(mnt, (const uint8_t*)"proc", 0555, v);
    if(e != M_SUCCESS) {
        return false;
    }

    int fd = fsopen("proc", FSOPEN_CLOEXEC);
    int err = errno;
    verbose_line(v, "proc", "fsopen()", fd < 0);
    if(fd < 0) {
        D_ERROR("Failed to open 'proc' filesystem context via fsopen()", err);
        return false;
    }

    int res = fsconfig(fd, FSCONFIG_SET_STRING, "hidepid", "2", 0);
    err     = errno;
    verbose_line(v, "proc hidepid", "2", res < 0);
    if(res < 0) {
        D_ERROR("Failed to set proc option 'hidepid=2' via fsconfig()", err);
        close(fd);
        return false;
    }

    res = fsconfig(fd, FSCONFIG_CMD_CREATE, NULL, NULL, 0);
    err = errno;
    verbose_line(v, "proc", "create", res < 0);
    if(res < 0) {
        D_ERROR("Failed to set proc option 'FSCONFIG_CMD_CREATE' via fsconfig()", err);
        close(fd);
        return false;
    }

    int proc = fsmount(fd, FSMOUNT_CLOEXEC, 0);
    err      = errno;
    verbose_line(v, "proc", "fsmount()", proc < 0);
    close(fd);
    if(proc < 0) {
        D_ERROR("Failed to create detached proc mount", err);
        return false;
    }

    res = move_mount(proc, "", mnt, "proc", MOVE_MOUNT_F_EMPTY_PATH);
    err = errno;
    verbose_line(v, "proc", "move_mount()", res < 0);
    close(proc);
    if(res < 0) {
        D_ERROR("Failed move_mount() proc mount onto tmpfs mount", err);
        return false;
    }

    return true;
}

bool setup_devices(int mnt, Verbose v) {
    uint8_t buff[PATH_MAX + 1];
    PathCtx ctx = {
        .mount_fd  = mnt,
        .buff      = buff,
        .buff_cap  = PATH_MAX + 1
    };

    v = verbose_set_title_width(v, 10);
    verbose_header(v, "Configuring: /dev");

    const char *const devs[5] = {
        "/dev/null",
        "/dev/zero",
        "/dev/full",
        "/dev/random",
        "/dev/urandom",
    };
    for(unsigned i = 0; i < 5; i++) {
        const char* dev = devs[i];

        ctx.env_dst  = (const uint8_t*)dev;
        ctx.host_src = (const uint8_t*)dev;

        MountErr e = bind_dev(&ctx, v);
        verbose_mnt_action(&ctx, v, e, "mount");
        if(e != M_SUCCESS) {
            print_mount_error(&ctx, e);
            return false;
        }
    }

    ctx.env_dst  = (const uint8_t*)"/dev/tty";
    ctx.host_src = (const uint8_t*)"/dev/tty";

    MountErr e = bind_dev(&ctx, v);
    verbose_mnt_action(&ctx, v, e, "mount");
    if(verbose_enabled(v) && e != M_SUCCESS) {
        print_mount_error(&ctx, e);
    }

    return true;
}

bool setup_libs(int mnt, Verbose v) {
    uint8_t buff[PATH_MAX + 1];
    PathCtx ctx = {
        .mount_fd  = mnt,
        .buff      = buff,
        .buff_cap  = PATH_MAX + 1
    };
    v = verbose_set_title_width(v, 10);
    verbose_header(v, "Configuring: libs");

    const char* links[][2] = {
        { "/lib",                   "usr/lib"          },
        { "/lib64",                 "usr/lib"          },
        { "/usr/lib32/libm.so",     "libm.so.6"        },
        { "/usr/lib64",             "lib"              },
        { "/usr/lib/ld-linux.so.2", "../ld-linux.so.2" },
        { nullptr,  nullptr }
    };

    const char* const libs[] = {
        "/usr/lib/ld-linux-x86-64.so.2",
        "/usr/lib/libc.so",
        "/usr/lib/libc.so.6",
        "/usr/lib/libm.so",
        "/usr/lib/libm.so.6",
        "/usr/lib/libgcc_s.so.1",

        "/usr/lib32/ld-linux.so.2",
        "/usr/lib32/libc.so",
        "/usr/lib32/libc.so.6",
        "/usr/lib32/libm.so.6",
        "/usr/lib32/libgcc_s.so.1",
        nullptr,
    };

    const char* const *lib = libs;
    while(*lib != nullptr) {
        const char* cur = *lib;

        ctx.env_dst  = (const uint8_t*)cur;
        ctx.host_src = (const uint8_t*)cur;

        MountErr e = bind_file(&ctx, v, true);
        verbose_mnt_action(&ctx, v, e, "mount");
        if(e != M_SUCCESS) {
            print_mount_error(&ctx, e);
            return false;
        }
        lib += 1;
    }

    const char *(*lnk)[2] = links;
    while((*lnk)[0] != nullptr) {
        ctx.env_dst  = (const uint8_t*)((*lnk)[0]);
        ctx.host_src = (const uint8_t*)((*lnk)[1]);

        MountErr e = make_symlink(&ctx, v);
        verbose_mnt_action(&ctx, v, e, "link");
        if(e != M_SUCCESS) {
            ctx.host_src = (const uint8_t*)"";
            print_mount_error(&ctx, e);
            return false;
        }
        lnk += 1;
    }
    return true;
}

bool swap_root_to_mount(int mnt, Verbose v) {
    verbose_header(v, "Swapping Process root ('/')");
    if(mnt < 0) {
        D_ERROR("Mount file descriptor is invalid");
        return false;
    }


    bool  ret     = false;
    int   fd      = -1;
    char* ptr     = nullptr;
    int   err     = 0;
    int   res     = 0;

    char  tmp[23] = "/tmp/sandy_root_XXXXXX";
    ptr = mkdtemp(tmp);
    err = errno;

    verbose_line(v, "mkdtemp", tmp, ptr == nullptr);
    if(ptr == nullptr) {
        D_ERROR("Failed to make temp directory for new root directory", err);
        goto done;
    }

    fd  = open("/tmp", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    err = errno;

    verbose_line(v, "open", "/tmp", fd < 0);
    if(fd < 0) {
        D_ERROR("Failed to open host '/tmp' directory", err);
        goto done;
    }

    res = move_mount(mnt, "", AT_FDCWD, ptr, MOVE_MOUNT_F_EMPTY_PATH);
    err = errno;
    
    verbose_line(v, "mount_mov", ptr, res < 0);
    if(res < 0) {
        D_ERROR("Failed move_mount() tmp directory for root", err);
        goto done;
    }

    // This man page: https://man7.org/linux/man-pages/man2/pivot_root.2.html
    // Shows this sequence
    // chdir(new_root);
    // pivot_root(".", ".");
    // umount2(".", MNT_DETACH);
    // It appears to me that basically old root ontop of mount point
    // so then when unmount is called it is the first to go.
    res = chdir(ptr);
    err = errno;

    verbose_line(v, "chdir", ptr, res < 0);
    if(res < 0) {
        // umount2 here because if this fails unlinkat will fail with dev busy.
        umount2(ptr, MNT_DETACH); 
        D_ERROR("Failed to chdir() to tmp directory for root", err);
        goto done;
    }

    res = pivot_root(".", ".");
    err = errno;

    verbose_line(v, "pivot_root", "pivot_root(\".\", \".\")", res < 0);
    if(res < 0) {
        // umount2 here because if this fails unlinkat will fail with dev busy.
        umount2(ptr, MNT_DETACH); 
        D_ERROR("pivot_root(\".\", \".\") failed", err);
        goto done;
    }

    res = umount2(".", MNT_DETACH);
    err = errno;

    verbose_line(v, "unmount", "old '/'", res < 0);
    if(res < 0) {
        D_ERROR("umount2(\".\", MNT_DETACH) failed", err);
        goto done;
    }
    // End of sequence shown in MAN page.

    res = chdir("/");
    err = errno;

    verbose_line(v, "chdir", "/", res < 0);
    if(res < 0) {
        D_ERROR("Failed to chdir() to new root", err);
        goto done;
    }
    ret = true;
done:
    if(mnt >= 0) {
        close(mnt);
    }
    if(fd >= 0 && ptr != nullptr) {
        res = unlinkat(fd, ptr + 5, AT_REMOVEDIR);
        err = errno;

        verbose_line(v, "unlinkat", ptr, res < 0);
        if(res < 0) {
            D_WARN("Failed to unlink tmp directory for root", err);
        }

        close(fd);
    } else if(ptr != nullptr) {
        res = rmdir(ptr);
        err = errno;

        verbose_line(v, "rmdir", ptr, res < 0);
        if(res < 0) {
            D_WARN("Failed to rmdir tmp directory for root.", err);
        }
    }

    return ret;
}

MountErr make_symlink(PathCtx* ctx, Verbose verbose) {
    int mnt = ctx->mount_fd;

    uint8_t buff[PATH_MAX];
    MountErr ret = prep_env_path(ctx->env_dst, buff, PATH_MAX);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkdir_env_parent(mnt, buff, verbose);
    if(ret != M_SUCCESS) {
        return ret;
    }

    int chk = symlinkat((const char*)ctx->host_src, mnt, (const char*)buff);
    int err = errno;
    if(chk >= 0) {
        return M_SUCCESS;
    }

    switch(err) {
        case EPERM:
        case EACCES:
            return M_ENV_PATH_PERM;
        case ENAMETOOLONG:
            return M_ENV_PATH_TOO_LONG;
        case ENOENT:
            return M_ENV_PATH_NOT_FOUND;
        case ELOOP:
        case ENOTDIR:
            return M_ENV_PATH_UNWALKABLE;
        case EEXIST:
            return M_ENV_PATH_EXISTS;
        case EROFS:
            return M_ENV_PATH_READ_ONLY;
        default:
            verbose_line(verbose, "symlinkat", (const char*)ctx->env_dst, V_FAIL);
            D_WARN("A system error occurred for symlinkat() for an environment path.", err);
            return M_ENV_PATH_SYS_ERR;
    }
}

MountErr bind_dev(PathCtx* ctx, Verbose verbose) {
    MountErr ret = prep_and_check_paths(ctx);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkdir_env_parent(ctx->mount_fd, ctx->buff, verbose);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkfile_env(ctx->mount_fd, ctx->buff, 0644, verbose, nullptr);
    if(ret != M_SUCCESS) {
        return ret;
    }

    int host = -1;
    ret = open_host_tree(ctx, OPEN_TREE_CLONE | OPEN_TREE_CLOEXEC, &host);
    if(ret != M_SUCCESS) {
        return ret;
    }

    if(!fd_is_dev(host)) {
        close(host);
        return M_HST_PATH_NOT_DEV;
    }

    struct mount_attr attr = {
        .attr_set = MOUNT_ATTR_NOSUID,
    };
    int res = mount_setattr(host, "", AT_EMPTY_PATH, &attr, sizeof(struct mount_attr));
    if(res < 0) {
        int err = errno;
        close(host);
        verbose_line(verbose, "mount_attr", (const char*)ctx->host_src, V_FAIL);
        D_WARN("A system error occurred for mount_setattr() for a host path.", err);
        return M_HST_PATH_SYS_ERR;
    }

    // Attach src file to mnt.
    res = move_mount(host, "", ctx->mount_fd, (const char*)ctx->buff, MOVE_MOUNT_F_EMPTY_PATH);
    if(res < 0) {
        int err = errno;
        close(host);
        verbose_line(verbose, "mount_mov", (const char*)ctx->host_src, V_FAIL);
        D_WARN("A system error occurred for move_mount() for a host path.", err);
        return M_HST_PATH_SYS_ERR;
    }

    return M_SUCCESS;
}

MountErr bind_dir(PathCtx* ctx, Verbose verbose, bool read_only) {
    MountErr ret = prep_and_check_paths(ctx);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkdir_env_parent(ctx->mount_fd, ctx->buff, verbose);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkdir_env(ctx->mount_fd, ctx->buff, 0755, verbose);
    if(ret != M_SUCCESS) {
        return ret;
    }

    int host = -1;
    ret = open_host_tree(ctx, AT_RECURSIVE | OPEN_TREE_CLONE | OPEN_TREE_CLOEXEC, &host);
    if(ret != M_SUCCESS) {
        return ret;
    }

    if(!fd_is_dir(host)) {
        close(host);
        return M_HST_PATH_NOT_DIR;
    }

    struct mount_attr attr = {
        .attr_set = MOUNT_ATTR_NOSUID | MOUNT_ATTR_NODEV,
    };
    if(read_only) {
        attr.attr_set |= MOUNT_ATTR_RDONLY;
    }
    
    int res = mount_setattr(host, "", AT_EMPTY_PATH | AT_RECURSIVE, &attr, sizeof(struct mount_attr));
    if(res < 0) {
        int err = errno;
        close(host);
        verbose_line(verbose, "mount_attr", (const char*)ctx->host_src, V_FAIL);
        D_WARN("A system error occurred for mount_setattr() for a host path.", err);
        return M_HST_PATH_SYS_ERR;
    }

    res = move_mount(host, "", ctx->mount_fd, (const char*)ctx->buff, MOVE_MOUNT_F_EMPTY_PATH);
    int err = errno;
    close(host);
    if(res < 0) {
        verbose_line(verbose, "mount_mov", (const char*)ctx->host_src, V_FAIL);
        D_WARN("A system error occurred for move_mount() for a host path.", err);
        return M_HST_PATH_SYS_ERR;
    }

    return M_SUCCESS;
}

MountErr bind_file(PathCtx* ctx, Verbose verbose, bool read_only) {
    MountErr ret = prep_and_check_paths(ctx);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkdir_env_parent(ctx->mount_fd, ctx->buff, verbose);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkfile_env(ctx->mount_fd, ctx->buff, 0644, verbose, nullptr);
    if(ret != M_SUCCESS) {
        return ret;
    }

    int host = -1;
    ret = open_host_tree(ctx, OPEN_TREE_CLONE | OPEN_TREE_CLOEXEC, &host);
    if(ret != M_SUCCESS) {
        return ret;
    }

    if(!fd_is_file(host)) {
        close(host);
        return M_HST_PATH_NOT_FILE;
    }

    struct mount_attr attr = {
        .attr_set = MOUNT_ATTR_NOSUID | MOUNT_ATTR_NODEV,
    };
    if(read_only) {
        attr.attr_set |= MOUNT_ATTR_RDONLY;
    }
    
    int res = mount_setattr(host, "", AT_EMPTY_PATH, &attr, sizeof(struct mount_attr));
    if(res < 0) {
        int err = errno;
        close(host);
        verbose_line(verbose, "mount_attr", (const char*)ctx->host_src, V_FAIL);
        D_WARN("A system error occurred for mount_setattr() for a host path.", err);
        return M_HST_PATH_SYS_ERR;
    }

    res = move_mount(host, "", ctx->mount_fd, (const char*)ctx->buff, MOVE_MOUNT_F_EMPTY_PATH);
    int err = errno;
    close(host);
    if(res < 0) {
        verbose_line(verbose, "mount_mov", (const char*)ctx->host_src, V_FAIL);
        D_WARN("A system error occurred for move_mount() for a host path.", err);
        return M_HST_PATH_SYS_ERR;
    }

    return M_SUCCESS;
}

static bool copy_fd(int dst, int src) {
    uint8_t buff[8192];
    for(;;) {
        ssize_t amt = read(src, buff, 8192);
        if(amt < 0) {
            if(errno == EINTR) {
                continue;
            }
            D_ERROR("A system error occurred for read(), when copying a file", errno);
            return false;
        }
        if(amt == 0) {
            break;
        }

        size_t   rem = amt;
        unsigned zer = 0;
        uint8_t* pos = buff;
        while(rem > 0 && zer < 10) {
            amt = write(dst, pos, rem);
            if(amt < 0) {
                if(errno == EINTR) {
                    continue;
                }
                D_ERROR("A system error occurred for write(), when copying a file", errno);
                return false;
            }
            zer += amt == 0;
            pos += amt;
            rem -= amt;
        }

        if(zer >= 10) {
            D_ERROR("Failed to write() when copying a file, probably no space left.");
            return false;
        }
    }
    return true;
}

MountErr copy_file(PathCtx* ctx, Verbose verbose, int mode) {
    MountErr ret = prep_and_check_paths(ctx);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkdir_env_parent(ctx->mount_fd, ctx->buff, verbose);
    if(ret != M_SUCCESS) {
        return ret;
    }

    int dst = -1;
    ret = mkfile_env(ctx->mount_fd, ctx->buff, 0644, verbose, &dst);
    if(ret != M_SUCCESS) {
        return ret;
    }

    int src = -1;
    ret = open_host(ctx, O_RDONLY, &src);
    if(ret != M_SUCCESS) {
        close(dst);
        return ret;
    }

    if(!fd_is_file(src)) {
        close(src);
        close(dst);
        return M_HST_PATH_NOT_FILE;
    }

    if(!copy_fd(dst, src)) {
        close(src);
        close(dst);
        return M_ENV_PATH_SYS_ERR;
    }
    close(src);

    if(mode != 0644) {
        if(fchmod(dst, mode) < 0) {
            D_WARN("A system error occurred for fchmod().", errno);
        }
    }
    close(dst);

    return M_SUCCESS;
}

void print_mount_error(PathCtx* ctx, MountErr error) {
    const char* env = ctx->env_dst  ? (const char*)ctx->env_dst : "(nullptr)";
    const char* hst = ctx->host_src ? (const char*)ctx->host_src : "(nullptr)";

    char buff[513] = { 0 };
    const char* out = buff;
    switch(error) {
        case M_SUCCESS:
            out = "Success: No Mounting error occurred\n";
            break;
        // Environment Path errors
        case M_ENV_PATH_NULL:
            snprintf(buff, 512, "Error: Environment mount path is null for: '%s'\n", hst);
            break;
        case M_ENV_PATH_EMPTY:
            snprintf(buff, 512, "Error: Environment mount path is empty for: '%s'\n", hst);
            break;
        case M_ENV_PATH_TOO_LONG:
            fprintf(stderr, "Error: Environment path too long: '%s'\n", env);
            return;
        case M_ENV_PATH_NOT_FOUND:
            snprintf(buff, 512, "Error: Environment path not found: '%s'\n", env);
            break;
        case M_ENV_PATH_UNWALKABLE:
            snprintf(buff, 512, "Error: A component of the environment path is not walkable: '%s'\n", env);
            break;
        case M_ENV_PATH_EXISTS:
            snprintf(buff, 512, "Error: Environment path already exists: '%s'\n", env);
            break;
        case M_ENV_PATH_NOT_DIR:
            snprintf(buff, 512, "Error: A component of the environment path is not a directory: '%s'\n", env);
            break;
        case M_ENV_PATH_PERM:
            snprintf(buff, 512, "Error: Permission denied to create environment path: '%s'\n", env);
            break;
        case M_ENV_PATH_READ_ONLY:
            snprintf(buff, 512, "Error: Environment path is on a read only file system: '%s'\n", env);
            break;
        case M_ENV_PATH_SYS_ERR:
            snprintf(buff, 512, "Error: Environment path encountered a system error: '%s'\n", env);
            break;
        // Host path errors
        case M_HST_PATH_NULL:
            snprintf(buff, 512, "Error: Host mount path is null for: '%s'\n", env);
            break;
        case M_HST_PATH_EMPTY:
            snprintf(buff, 512, "Error: Host mount path is empty for: '%s'\n", env);
            break;
        case M_HST_PATH_TOO_LONG:
            fprintf(stderr, "Error: Host path too long: '%s'\n", hst);
            return;
        case M_HST_PATH_NOT_FOUND:
            snprintf(buff, 512, "Error: Host path not found: '%s'\n", hst);
            break;
        case M_HST_PATH_UNWALKABLE:
            snprintf(buff, 512, "Error: A component of the host path is not walkable: '%s'\n", hst);
            break;
        case M_HST_PATH_NOT_DEV:
            snprintf(buff, 512, "Error: Host path is not a device: '%s'\n", hst);
            break;
        case M_HST_PATH_NOT_DIR:
            snprintf(buff, 512, "Error: Host path is not a directory: '%s'\n", hst);
            break;
        case M_HST_PATH_NOT_FILE:
            snprintf(buff, 512, "Error: Host path is not a file: '%s'\n", hst);
            break;
        case M_HST_PATH_PERM:
            snprintf(buff, 512, "Error: Permission denied to host path: '%s'\n", hst);
            break;
        case M_HST_PATH_SYS_ERR:
            snprintf(buff, 512, "Error: Host path encountered a system error: '%s'\n", hst);
            break;
        default:
            snprintf(buff, 512, "Error: Unknown mount error: %" PRIu32 "\n", error);
            break;
    }
    
    size_t len = strlen(out);
    fwrite(out, 1, len, stderr);
}

void verbose_mnt_action(PathCtx* ctx, Verbose verbose, MountErr err, const char* action) {
    if(!verbose_enabled(verbose)) {
        return;
    }
    char buff[PATH_MAX + 1];
    snprintf(buff, PATH_MAX, "host: %s", ctx->host_src);
    verbose_line(verbose, action, buff, err != M_SUCCESS);
    snprintf(buff, PATH_MAX, "env:  %s", ctx->env_dst);
    verbose_line(verbose, "", buff, V_NONE);
}

static bool write_str(const char *path, const char *str) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if(fd < 0) {
        return false;
    }

    const char *pos = str;
    size_t rem = strlen(str);
    while(rem > 0) {
        ssize_t amt = write(fd, pos, rem);
        if(amt >= 0) {
            pos += amt;
            rem -= amt;
            continue;
        }

        int old = errno;
        if(old == EINTR) {
            continue;
        }

        close(fd);
        // restore errno
        errno = old;
        return false;
    }
    close(fd);
    return true;
}

static bool write_ids(uid_t uid, gid_t gid) {
    char buff[32] = { 0 };

    // Needed so I can write to the map files.
    if(!write_str("/proc/self/setgroups", "deny\n")) {
        D_ERROR("Failed write 'deny' to '/proc/self/setgroups'", errno);
        return false;
    }

    snprintf(buff, 32, "0 %u 1\n", uid);
    if(!write_str("/proc/self/uid_map", buff)) {
        D_ERROR("Failed writing to '/proc/self/uid_map'", errno);
        return false;
    }

    snprintf(buff, 32, "0 %u 1\n", gid);
    if(!write_str("/proc/self/gid_map", buff)) {
        D_ERROR("Failed writing to '/proc/self/gid_map'", errno);
        return false;
    }
    return true;
}

static bool enter_namespace(void) {
    uid_t uid = geteuid();
    gid_t gid = getegid();

    if(unshare(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID) < 0) {
        const char *msg = "Failed to make a new mount, PID, and user namespace";
        if (errno == EPERM) {
            msg = "Failed to make a new mount, PID, and user namespace (user namespaces are probably restricted)";
        } else if (errno == EINVAL) {
            msg = "Failed to make a new mount, PID, and user namespace (user namespaces maybe unsupported)";
        } else if (errno == ENOSPC) {
            msg = "Failed to make a new mount, PID, and user namespace (namespaces limit reached)";
        }
        D_ERROR(msg, errno);
        return false;
    }

    if(!write_ids(uid, gid)) {
        return false;
    }

    // Change mount point propagation for root to private.
    if(mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) {
        D_ERROR("Failed to make process mounts private", errno);
        return false;
    }
    return true;
}

static const uint8_t* trim_leading_slashes(const uint8_t* str) {
    if(str == nullptr) {
        return nullptr;
    }
    while(*str == '/') {
        str++;
    }
    return str;
}

static MountErr prep_env_path(const uint8_t* path, uint8_t buff[], size_t blen) {
    if(path == nullptr) {
        return M_ENV_PATH_NULL;
    }
    if(blen == 0) {
        return M_ENV_PATH_TOO_LONG;
    }
    memset(buff, 0, blen);
    // This is still nessacry because not all <name>at() calls are like
    // openat2 where I can pass RESOLVE_IN_ROOT. So we want to make sure
    // the environment path is relative. (e.g mkdirat)
    path = trim_leading_slashes(path);

    // Copy path into the buffer.
    size_t len = 0;
    while(len < blen) {
        uint8_t cur = path[len];
        buff[len] = cur;
        if(cur == '\0') {
            break;
        }
        len += 1;
    }
    if(len >= blen) {
        // add null just incase I print it or something.
        buff[blen - 1] = '\0'; 
        return M_ENV_PATH_TOO_LONG;
    }

    // Trim any trailing slashes
    uint8_t* end = buff + len;
    while(end > buff && end[-1] == '/') {
        end -= 1;
        *end = '\0';
    }

    if(buff[0] == '\0') {
        return M_ENV_PATH_EMPTY;
    }

    return M_SUCCESS;
}

static MountErr check_host_path(const uint8_t* path) {
    if(path == nullptr) {
        return M_HST_PATH_NULL;
    }
    if(path[0] == '\0') {
        return M_HST_PATH_EMPTY;
    }
    return M_SUCCESS;
}

static MountErr prep_and_check_paths(PathCtx* ctx) {
    MountErr ret = check_host_path(ctx->host_src);
    if(ret != M_SUCCESS) {
        return ret;
    }
    return prep_env_path(ctx->env_dst, ctx->buff, ctx->buff_cap);
}

static bool fd_is_dev(int fd) {
    struct stat path_stat = { 0 };
    if (fstat(fd, &path_stat) != 0) {
        return false;
    }
    return S_ISBLK(path_stat.st_mode) | S_ISCHR(path_stat.st_mode);
}

static bool fd_is_dir(int fd) {
    struct stat path_stat = { 0 };
    if (fstat(fd, &path_stat) != 0) {
        return false;
    }
    return S_ISDIR(path_stat.st_mode) != 0;
}

static bool fd_is_file(int fd) {
    struct stat path_stat = { 0 };
    if (fstat(fd, &path_stat) != 0) {
        return false;
    }
    return S_ISREG(path_stat.st_mode) != 0;
}

static bool is_dirat(int dirfd, const uint8_t *path) {
    if(path == nullptr) {
        return false;
    }
    struct stat path_stat = { 0 };
    if(fstatat(dirfd, (const char*)path, &path_stat, 0) != 0) {
        return false;
    }
    return S_ISDIR(path_stat.st_mode) != 0;
}

static MountErr open_host_tree(PathCtx* ctx, unsigned flags, int* fd_out) {
    int fd = open_tree(AT_FDCWD, (const char*)(ctx->host_src), flags | OPEN_TREE_CLOEXEC);
    if(fd >= 0) {
        *fd_out = fd;
        return M_SUCCESS;
    }
    int errnum = errno;
    switch(errnum) {
        case EPERM:
        case EACCES:
            return M_HST_PATH_PERM;
        case ENAMETOOLONG:
            return M_HST_PATH_TOO_LONG;
        case ENOENT:
            return M_HST_PATH_NOT_FOUND;
        case ELOOP:
        case ENOTDIR:
            return M_HST_PATH_UNWALKABLE;
        default:
            D_WARN("A system error occurred for open_tree() for a host path.", errnum);
            return M_HST_PATH_SYS_ERR;
    }
}

static MountErr open_host(PathCtx* ctx, unsigned flags, int* fd_out) {
    int fd = open((const char*)(ctx->host_src), flags | O_CLOEXEC);
    if(fd >= 0) {
        *fd_out = fd;
        return M_SUCCESS;
    }
    int errnum = errno;
    switch(errnum) {
        case EPERM:
        case EACCES:
            return M_HST_PATH_PERM;
        case ENAMETOOLONG:
            return M_HST_PATH_TOO_LONG;
        case ENOENT:
            return M_HST_PATH_NOT_FOUND;
        case ELOOP:
        case ENOTDIR:
            return M_HST_PATH_UNWALKABLE;
        default:
            D_WARN("A system error occurred for open() for a host path.", errnum);
            return M_HST_PATH_SYS_ERR;
    }
}

static MountErr mkdir_env(int mnt, const uint8_t* path, mode_t mode, Verbose v) {
    int res = mkdirat(mnt, (const char*)path, mode);
    if(res >= 0) {
        verbose_line(v, "mkdir_env", (const char*)path, V_OK);
        return M_SUCCESS;
    }

    int errnum = errno;
    // If it was a directory count it as a success.
    // Also don't want to print vebose for existing dirs.
    if(errnum == EEXIST && is_dirat(mnt, path)) {
        return M_SUCCESS;
    }

    verbose_line(v, "mkdir_env", (const char*)path, V_FAIL);

    switch(errnum) {
        case EPERM:
        case EACCES:
            return M_ENV_PATH_PERM;
        case ENAMETOOLONG:
            return M_ENV_PATH_TOO_LONG;
        case ENOENT:
            return M_ENV_PATH_NOT_FOUND;
        case ELOOP:
        case ENOTDIR:
            return M_ENV_PATH_UNWALKABLE;
        case EEXIST:
            return M_ENV_PATH_NOT_DIR;
        case EROFS:
            return M_ENV_PATH_READ_ONLY;
        default:
            D_WARN("A system error occurred for mkdirat() for an environment path.", errnum);
            return M_ENV_PATH_SYS_ERR;
    }
}

static MountErr mkfile_env(int mnt, const uint8_t* path, mode_t mode, Verbose v, int* fd_out) {
    struct open_how how = {
        .flags   = O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW | O_CLOEXEC,
        .mode    = mode,
        .resolve = RESOLVE_IN_ROOT
    }; 
    int res = openat2(mnt, (const char*)path, &how, sizeof(struct open_how));
    if(res >= 0) {
        if(fd_out != nullptr) {
            *fd_out = res;
        } else {
            close(res);
        }
        verbose_line(v, "mkfile_env", (const char*)path, V_OK);
        return M_SUCCESS;
    }

    int errnum = errno;
    verbose_line(v, "mkfile_env", (const char*)path, V_FAIL);

    switch(errnum) {
        case EPERM:
        case EACCES:
            return M_ENV_PATH_PERM;
        case ENAMETOOLONG:
            return M_ENV_PATH_TOO_LONG;
        case ENOENT:
            return M_ENV_PATH_NOT_FOUND;
        case ELOOP:
        case ENOTDIR:
            return M_ENV_PATH_UNWALKABLE;
        case EEXIST:
            return M_ENV_PATH_EXISTS;
        case EROFS:
            return M_ENV_PATH_READ_ONLY;
        default:
            D_WARN("A system error occurred for mkdirat() for an environment path.", errnum);
            return M_ENV_PATH_SYS_ERR;
    }
}

// Make all the parent directories of the environment path.
// Note path buff to be populated with a string.
static MountErr mkdir_env_parent(int mnt, uint8_t* buf, Verbose v) {
    for(uint8_t* cur = buf; *cur != '\0'; cur++) {
        if(*cur != '/') {
            continue;
        }
        // Shorten string.
        *cur = '\0';

        MountErr e = mkdir_env(mnt, buf, 0755, v);

        // Restore string.
        *cur = '/';
        
        if(e != M_SUCCESS) {
            return e;
        }
    }
    return M_SUCCESS;
}