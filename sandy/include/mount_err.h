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
    M_HST_PATH_TOO_LONG,
    M_HST_PATH_NOT_FOUND,
    M_HST_PATH_UNWALKABLE,
    M_HST_PATH_NOT_DEV,
    M_HST_PATH_PERM,
    M_HST_PATH_SYS_ERR,
} MountErr;

#endif