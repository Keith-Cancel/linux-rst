#ifndef MOUNT_UTIL_H
#define MOUNT_UTIL_H

int setup_mount();
int bind_dir(int mnt, const char *host_src, const char *target, bool read_only);

#endif