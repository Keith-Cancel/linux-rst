#ifndef _MOUNT_ERROR_H
#define _MOUNT_ERROR_H

#include <stdint.h>

typedef enum : uint32_t {
    M_SUCCESS = 0,
    M_ENV_PATH_NULL,
    M_ENV_PATH_EMPTY,
    M_ENV_PATH_TOO_LONG,
    M_ENV_PATH_BAD_PARENT,
    M_HST_PATH_NULL,
    M_HST_PATH_EMPTY,
} MountErr;

#endif