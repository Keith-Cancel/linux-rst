#ifndef _MOUNT_UTIL_H
#define _MOUNT_UTIL_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "verbose.h"

typedef struct path_ctx_s {
    const uint8_t* host_src;
    const uint8_t* env_dst;
    uint8_t*       buff;
    size_t         buff_cap;
    int            mount_fd;
} PathCtx;

typedef enum : uint32_t {
    M_SUCCESS = 0,

    M_ENV_PATH_NULL,
    M_ENV_PATH_EMPTY,
    M_ENV_PATH_TOO_LONG,
    M_ENV_PATH_NOT_FOUND,
    M_ENV_PATH_UNWALKABLE,
    M_ENV_PATH_EXISTS,
    M_ENV_PATH_NOT_DIR,
    M_ENV_PATH_PERM,
    M_ENV_PATH_READ_ONLY,
    M_ENV_PATH_SYS_ERR,

    M_HST_PATH_NULL,
    M_HST_PATH_EMPTY,
    M_HST_PATH_TOO_LONG,
    M_HST_PATH_NOT_FOUND,
    M_HST_PATH_UNWALKABLE,
    M_HST_PATH_NOT_DEV,
    M_HST_PATH_NOT_DIR,
    M_HST_PATH_NOT_FILE,
    M_HST_PATH_PERM,
    M_HST_PATH_SYS_ERR,
} MountErr;

int  setup_mount(Verbose verbose);
bool setup_devices(int mnt, Verbose verbose);
bool setup_libs(int mnt, Verbose verbose);
// Swap the root directory to the mount and close the mount file descriptor.
bool swap_root_to_mount(int mnt, Verbose v);

MountErr bind_dev    (PathCtx* ctx, Verbose verbose);
MountErr bind_dir    (PathCtx* ctx, Verbose verbose, bool read_only);
MountErr bind_file   (PathCtx* ctx, Verbose verbose, bool read_only);
MountErr copy_file   (PathCtx* ctx, Verbose verbose, int mode);
MountErr make_symlink(PathCtx* ctx, Verbose verbose);

void print_mount_error(PathCtx* ctx, MountErr error);
void verbose_mnt_action(PathCtx* ctx, Verbose verbose, MountErr err, const char* action);

#endif