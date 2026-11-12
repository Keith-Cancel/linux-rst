#define _GNU_SOURCE
#include "mount.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>

static int mkdirat_p(int mnt, const char* path);
static const char* skip_slashes(const char* str);

int setup_mount() {
    int fd = fsopen("tmpfs", FSOPEN_CLOEXEC);
    if(fd < 0) {
        perror("fsopen");
        return -1;
    }
    if(fsconfig(fd, FSCONFIG_SET_STRING, "size", "128M", 0) < 0) {
        perror("fsconfig size");
        close(fd);
        return -1;
    }
    if(fsconfig(fd, FSCONFIG_SET_STRING, "mode", "0755", 0) < 0) {
        perror("fsconfig mode");
        close(fd);
        return -1;
    }
    if (fsconfig(fd, FSCONFIG_CMD_CREATE_EXCL, NULL, NULL, 0) < 0) {
        perror("fsconfig FSCONFIG_CMD_CREATE_EXCL");
        close(fd);
        return -1;
    }
    int mnt = fsmount(fd, FSMOUNT_CLOEXEC, 0);
    close(fd);
    if(mnt < 0) { 
        perror("fsmount");
        return -1;
    }
    return mnt;
}

int bind_dir(int mnt, const char *host_src, const char *target, bool read_only) {
    if (host_src == nullptr || host_src[0] == '\0') {
        fprintf(stderr, "host_src path must be a non-empty path");
        return -1;
    }
    target = skip_slashes(target);
    if (target == nullptr || target[0] == '\0') {
        fprintf(stderr, "target path must be a non-empty path");
        return -1;
    }
    if(mkdirat_p(mnt, target) != 0) {
        return -1;
    }
    if(mkdirat(mnt, target, 0755) < 0) {
        perror("mkdirat");
        return -1;
    }

    int src = open_tree(AT_FDCWD, host_src, OPEN_TREE_CLONE | OPEN_TREE_CLOEXEC | AT_RECURSIVE);
    if(src < 0) {
        perror("open_tree");
        return -1;
    }

    struct mount_attr attr = {
        .attr_set = MOUNT_ATTR_NOSUID | MOUNT_ATTR_NODEV,
    };
    if (read_only) {
        attr.attr_set |= MOUNT_ATTR_RDONLY;
    }
    if (mount_setattr(src, "", AT_EMPTY_PATH | AT_RECURSIVE, &attr, sizeof(attr)) < 0) {
        perror("mount_setattr");
        close(src);
        return -1;
    }
    // Attach src path to mnt.
    if (move_mount(src, "", mnt, target, MOVE_MOUNT_F_EMPTY_PATH) < 0) {
        perror("move_mount");
        close(src);
        return -1;
    }
    close(src);
    return 0;
}

static const char* skip_slashes(const char* str) {
    if(str == nullptr) {
        return nullptr;
    }
    while(*str == '/') {
        str++;
    }
    return str;
}

static int mkdirat_p(int mnt, const char* path) {
    char p_cpy[PATH_MAX];
    // Copy path to a buffer so I can modify it as I go.
    size_t len = strlen(path);
    if(len >= (PATH_MAX - 1)) {
        fprintf(stderr, "Path too long: '%s'`\n", path);
        return -1;
    }
    memcpy(p_cpy, path, len);
    p_cpy[len] = '\0';

    char* p = p_cpy;
    while(*p != '\0') {
        if(*p == '/') {
            *p = '\0';
            int res = mkdirat(mnt, p_cpy, 0755);
            if(res < 0 && errno != EEXIST) {
                perror("mkdirat p");
                return -1;
            }
            *p = '/';
        }
        p += 1;
    }
    return 0;
}
