#ifndef _MOUNT_UTIL_H
#define _MOUNT_UTIL_H

#include <stdbool.h>
#include <stdlib.h>
#include "mount_err.h"
#include "verbose.h"

typedef struct path_ctx_s {
    const uint8_t* host_path;
    const uint8_t* env_path;
    uint8_t*       buff;
    size_t         buff_cap;
    int            mount_fd;
} PathCtx;

int  setup_mount(Verbose verbose);
bool setup_devices(int mnt, Verbose verbose);

MountErr bind_dev(int mnt, PathCtx* ctx, Verbose verbose);
int bind_dev_old(int mnt, const char* host_dev, const char* target);

// Swap the root directory to the mount and close the mount file descriptor.
int swap_root_to_mount(int mnt);
int bind_dir(int mnt, const char *host_src, const char *target, bool read_only);
int bind_file(int mnt, const char* host_file, const char* target, bool read_only);

#endif