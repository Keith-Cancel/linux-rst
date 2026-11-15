#define _GNU_SOURCE
#include "diag.h"
#include "mount.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>

static bool fd_is_dev      (int fd);
static bool enter_namespace(void);

static MountErr open_host_tree       (PathCtx* ctx, unsigned flags, int* fd_out);
static MountErr mkfile_env           (int mnt, const uint8_t* path, mode_t mode, Verbose verbose);
static MountErr mkdir_env_parent     (int mnt, uint8_t* buf, Verbose verbose);
static MountErr prep_and_check_paths (PathCtx* ctx);


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

    int res = fsconfig(fd, FSCONFIG_SET_STRING, "size", "128M", 0);
    err     = errno;
    verbose_line(v, "tmpfs size", "128M", res < 0);
    if(res < 0) {
        D_ERROR("Failed to set tmpfs option 'size=128M' via fsconfig()", err);
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

MountErr bind_dev(PathCtx* ctx, Verbose verbose) {
    MountErr ret = prep_and_check_paths(ctx);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkdir_env_parent(ctx->mount_fd, ctx->buff, verbose);
    if(ret != M_SUCCESS) {
        return ret;
    }

    ret = mkfile_env(ctx->mount_fd, ctx->buff, 0644, verbose);
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
        verbose_line(verbose, "mount", (const char*)ctx->host_path, V_FAIL);
        D_WARN("A system error occurred for mount_setattr() for a host path.", err);
        return M_HST_PATH_SYS_ERR;
    }

    // Attach src file to mnt.
    res = move_mount(host, "", ctx->mount_fd, (const char*)ctx->buff, MOVE_MOUNT_F_EMPTY_PATH);
    if(res < 0) {
        int err = errno;
        close(host);
        verbose_line(verbose, "mount", (const char*)ctx->host_path, V_FAIL);
        D_WARN("A system error occurred for move_mount() for a host path.", err);
        return M_HST_PATH_SYS_ERR;
    }

    return M_SUCCESS;
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

    if(unshare(CLONE_NEWUSER | CLONE_NEWNS) < 0) {
        const char *msg = "Failed to make a new mount and user namespace";
        if (errno == EPERM) {
            msg = "Failed to make a new mount and user namespace (user namespaces are probably restricted)";
        } else if (errno == EINVAL) {
            msg = "Failed to make a new mount and user namespace (user namespaces maybe unsupported)";
        } else if (errno == ENOSPC) {
            msg = "Failed to make a new mount and user namespace (namespaces limit reached)";
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
    MountErr ret = check_host_path(ctx->host_path);
    if(ret != M_SUCCESS) {
        return ret;
    }
    return prep_env_path(ctx->env_path, ctx->buff, ctx->buff_cap);
}

static bool fd_is_dev(int fd) {
    struct stat path_stat = { 0 };
    if (fstat(fd, &path_stat) != 0) {
        return false;
    }
    return S_ISBLK(path_stat.st_mode) | S_ISCHR(path_stat.st_mode);
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
    int fd = open_tree(AT_FDCWD, (const char*)(ctx->host_path), flags | OPEN_TREE_CLOEXEC);
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

static MountErr mkfile_env(int mnt, const uint8_t* path, mode_t mode, Verbose v) {
    int res = openat(mnt, (const char*)path, O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW | O_CLOEXEC , mode);
    if(res >= 0) {
        close(res);
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