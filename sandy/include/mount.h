#ifndef MOUNT_UTIL_H
#define MOUNT_UTIL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum : uint32_t {
    SUCCESS = 0,
    ENV_PATH_NULL,
    ENV_PATH_EMPTY,
    ENV_PATH_TOO_LONG,
    HST_PATH_NULL,
    HST_PATH_EMPTY,
} MountErr;

int  setup_mount(bool verbose);
bool setup_devices(int mnt, bool verbose);

int bind_dev(int mnt, const char* host_dev, const char* target);
// Swap the root directory to the mount and close the mount file descriptor.
int swap_root_to_mount(int mnt);
int bind_dir(int mnt, const char *host_src, const char *target, bool read_only);
int bind_file(int mnt, const char* host_file, const char* target, bool read_only);

#endif