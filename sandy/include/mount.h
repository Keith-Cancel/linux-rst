#ifndef MOUNT_UTIL_H
#define MOUNT_UTIL_H

#include <stdbool.h>

int setup_mount(bool verbose);
int setup_devices(int mnt);
int bind_dev(int mnt, const char* host_dev, const char* target);
// Swap the root directory to the mount and close the mount file descriptor.
int swap_root_to_mount(int mnt);
int bind_dir(int mnt, const char *host_src, const char *target, bool read_only);
int bind_file(int mnt, const char* host_file, const char* target, bool read_only);

#endif