#define _GNU_SOURCE
#include "mount.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syscall.h>

static int pivot_root(const char *new_root, const char *put_old) {
    return syscall(SYS_pivot_root, new_root, put_old);
}

static bool is_dir(const char *path);
static bool is_file(const char *path);
static int enter_namespace(void);
static int bind_path(int mnt, const char *host_path, const char *target, struct mount_attr* attr, bool dir, bool silent);

int setup_mount(void) {
    if (enter_namespace() < 0) {
        return -1;
    }

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
    if(fsconfig(fd, FSCONFIG_CMD_CREATE_EXCL, NULL, NULL, 0) < 0) {
        perror("fsconfig FSCONFIG_CMD_CREATE_EXCL");
        close(fd);
        return -1;
    }
    int mnt = fsmount(fd, FSMOUNT_CLOEXEC, MOUNT_ATTR_NOSUID | MOUNT_ATTR_NODEV);
    close(fd);
    if(mnt < 0) {
        perror("fsmount");
        return -1;
    }
    return mnt;
}

int setup_devices(int mnt) {
    const char *const devs[] = {
        "/dev/null",
        "/dev/zero",
        "/dev/full",
        "/dev/random",
        "/dev/urandom",
    };
    for(unsigned i = 0; i < 5; i++) {
        if(bind_dev(mnt, devs[i], devs[i]) < 0) {
            return -1;
        }
    }
    
    // Failing is fine.
    bind_path(mnt, "/dev/tty", "dev/tty", &(struct mount_attr) { .attr_set = MOUNT_ATTR_NOSUID }, false, true);

    return 0;
}

int bind_dir(int mnt, const char *host_src, const char *target, bool read_only) {
    if(!is_dir(host_src)) {
        fprintf(stderr, "'%s' is not a directory.\n", host_src);
        return -1;
    }
    struct mount_attr attr = {
        .attr_set = MOUNT_ATTR_NOSUID | MOUNT_ATTR_NODEV,
    };
    if(read_only) {
        attr.attr_set |= MOUNT_ATTR_RDONLY;
    }

    return bind_path(mnt, host_src, target, &attr, true, false);
}

int bind_file(int mnt, const char* host_file, const char* target, bool read_only) {
    if(!is_file(host_file)) {
        fprintf(stderr, "'%s' is not a file.\n", host_file);
        return -1;
    }
    struct mount_attr attr = {
        .attr_set = MOUNT_ATTR_NOSUID | MOUNT_ATTR_NODEV,
    };
    if(read_only) {
        attr.attr_set |= MOUNT_ATTR_RDONLY;
    }
    return bind_path(mnt, host_file, target, &attr, false, false);
}

int bind_dev(int mnt, const char* host_dev, const char* target) {
    struct mount_attr attr = {
        .attr_set = MOUNT_ATTR_NOSUID,
    };
    return bind_path(mnt, host_dev, target, &attr, false, false);
}

int swap_root_to_mount(int mnt) {
    int tmp_fd = - 1;
    if(mnt < 0) {
        fprintf(stderr, "invalid mount file descriptor\n");
        return -1;
    }

    // Create a directory to mount the tmpfs.
    char tmp[23] = "/tmp/sandy_root_XXXXXX";
    if(mkdtemp(tmp) == NULL) {
        perror("mkdtemp");
        goto fail;
    }
    
    tmp_fd = open(tmp, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if(tmp_fd < 0) {
        perror("open tmp");
        goto fail;
    }

    if(move_mount(mnt, "", AT_FDCWD, tmp, MOVE_MOUNT_F_EMPTY_PATH) < 0) {
        perror("move_mount tmp root");
        goto fail;
    }
    // Mount has been attached so close we can close it.
    close(mnt);
    mnt = -1;

    // This man page: https://man7.org/linux/man-pages/man2/pivot_root.2.html
    // Shows this sequence
    // chdir(new_root);
    // pivot_root(".", ".");
    // umount2(".", MNT_DETACH);
    // It appears to me that basically old root ontop of mount point
    // so then when unmount is called it is the first to go.

    if(chdir(tmp) < 0) {
        perror("chdir new root");
        goto fail;
    }

    if (pivot_root(".", ".") < 0) {
        perror("pivot_root");
        goto fail;
    }

    if(umount2(".", MNT_DETACH) < 0) {
        perror("umount2");
        goto fail;
    }

    if(chdir("/") < 0) {
        perror("chdir '/'");
        goto fail;
    }

    unlinkat(tmp_fd, ".", AT_REMOVEDIR);
    close(tmp_fd);

    return 0;
fail:
    if(mnt >= 0) {
        close(mnt);
    }
    if(tmp_fd >= 0) {
        unlinkat(tmp_fd, ".", AT_REMOVEDIR);
        close(tmp_fd);
    }
    return -1;
}

static int write_str(const char *path, const char *str) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if(fd < 0) {
        perror(path);
        return -1;
    }

    const char *pos = str;
    size_t rem = strlen(str);
    while(rem > 0) {
        ssize_t amt = write(fd, pos, rem);
        if(amt < 0) {
            if(errno == EINTR) {
                continue;
            }
            close(fd);
            return -1;
        }
        pos += amt;
        rem -= amt;
    }
    close(fd);
    return 0;
}

static int write_ids(uid_t uid, gid_t gid) {
    char buff[32] = { 0 };

    // Needed so I can write to the map files.
    if(write_str("/proc/self/setgroups", "deny\n") < 0) {
        return -1;
    }

    snprintf(buff, 32, "0 %u 1\n", uid);
    if(write_str("/proc/self/uid_map", buff) < 0) {
        return -1;
    }

    snprintf(buff, 32, "0 %u 1\n", gid);
    if(write_str("/proc/self/gid_map", buff) < 0) {
        return -1;
    }
    return 0;
}

static int enter_namespace(void) {
    uid_t uid = geteuid();
    gid_t gid = getegid();

    if(unshare(CLONE_NEWUSER | CLONE_NEWNS) < 0) {
        perror("unshare CLONE_NEWUSER | CLONE_NEWNS");
        return -1;
    }

    if(write_ids(uid, gid) < 0) {
        return -1;
    }

    // Change mount point propagation for root to private.
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) {
        perror("mount MS_REC | MS_PRIVATE");
        return -1;
    }
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

void sperror(const char* msg, bool silent) {
    if(!silent) {
        perror(msg);
    }
}

static int mkdirat_p(int mnt, const char* path, bool silent) {
    char p_cpy[PATH_MAX];
    // Copy path to a buffer so I can modify it as I go.
    size_t len = strlen(path);
    if(len >= (PATH_MAX - 1)) {
        if(!silent) {
            fprintf(stderr, "Path too long: '%s'`\n", path);
        }
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
                sperror("mkdirat_p", silent);
                return -1;
            }
            *p = '/';
        }
        p += 1;
    }
    return 0;
}

static bool is_dir(const char *path) {
    if(path == nullptr) {
        return false;
    }

    struct stat path_stat = { 0 };
    if (stat(path, &path_stat) != 0) {
        return false; 
    }

    return S_ISDIR(path_stat.st_mode) != 0;
}

static bool is_file(const char *path) {
    if(path == nullptr) {
        return false;
    }

    struct stat path_stat = { 0 };
    if (stat(path, &path_stat) != 0) {
        return false; 
    }
    
    return S_ISREG(path_stat.st_mode);
}

static int bind_path(int mnt, const char *host_path, const char *target, struct mount_attr* attr, bool dir, bool silent) {
    if(host_path == nullptr || host_path[0] == '\0') {
        if(!silent) {
            fprintf(stderr, "host path must be a non-empty path\n");
        }
        return -1;
    }
    target = skip_slashes(target);
    if(target == nullptr || target[0] == '\0') {
        if(!silent) {
            fprintf(stderr, "target path must be a non-empty path\n");
        }
        return -1;
    }
    if(mkdirat_p(mnt, target, silent) != 0) {
        return -1;
    }
    
    unsigned flags1 = AT_EMPTY_PATH;
    unsigned flags0 = OPEN_TREE_CLONE | OPEN_TREE_CLOEXEC;
    int res = -1;
    if(dir) {
        res = mkdirat(mnt, target, 0755);
        flags0 |= AT_RECURSIVE;
        flags1 |= AT_RECURSIVE;
    } else {
        res = openat(mnt, target, O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW | O_CLOEXEC , 0644);
        if(res >= 0) {
            close(res);
        }
    }
    if(res < 0) {
        sperror("openat/mkdirat", silent);
        return -1;
    }

    int src = open_tree(AT_FDCWD, host_path, flags0);
    if (src < 0) {
        sperror("open_tree", silent);
        return -1;
    }

    if(mount_setattr(src, "", flags1, attr, sizeof(struct mount_attr)) < 0) {
        sperror("fstat", silent);
        close(src);
        return -1;
    }

    // Attach src file to mnt.
    if (move_mount(src, "", mnt, target, MOVE_MOUNT_F_EMPTY_PATH) < 0) {
        sperror("move_mount dev", silent);
        close(src);
        return -1;
    }

    close(src);
    return 0;
}