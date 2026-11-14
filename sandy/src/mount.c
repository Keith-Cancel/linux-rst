#define _GNU_SOURCE
#include "diag.h"
#include "mount.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/mount.h>

static bool enter_namespace(void);

int setup_mount(Verbose v) {
    v = verbose_set_title_width(v, 10);
    verbose_header(v, "Configuring: tmpfs");
    if (!enter_namespace()) {
        return -1;
    }

    int fd = fsopen("tmpfs", FSOPEN_CLOEXEC);
    verbose_line(v, "tmpfs", "fsopen()", fd < 0);
    if(fd < 0) {
        perror("tmpfs: fsopen");
        return -1;
    }

    int res = fsconfig(fd, FSCONFIG_SET_STRING, "size", "128M", 0);
    verbose_line(v, "tmpfs size", "128M", res < 0);
    if(res < 0) {
        perror("tmpfs: size");
        close(fd);
        return -1;
    }

    res = fsconfig(fd, FSCONFIG_SET_STRING, "mode", "0755", 0);
    verbose_line(v, "tmpfs mode", "0755", res < 0);
    if(res < 0) {
        perror("tmpfs: mode");
        close(fd);
        return -1;
    }

    res = fsconfig(fd, FSCONFIG_CMD_CREATE_EXCL, NULL, NULL, 0);
    verbose_line(v, "tmpfs", "exclusive", res < 0);
    if(res < 0) {
        perror("tmpfs: exclusive");
        close(fd);
        return -1;
    }

    int mnt = fsmount(fd, FSMOUNT_CLOEXEC, MOUNT_ATTR_NOSUID | MOUNT_ATTR_NODEV);
    verbose_line(v, "tmpfs", "fsmount()", mnt < 0);
    close(fd);
    if(mnt < 0) {
        perror("tmpfs: mount");
        return -1;
    }
    return mnt;
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
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) {
        D_ERROR("Failed to make process mounts private", errno);
        return false;
    }
    return true;
}