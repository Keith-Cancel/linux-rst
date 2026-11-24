#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "fldr.h"
#include "flex_arr.h"
#include "xmem.h"

typedef struct file_data_s {
    uint8_t* data;
    size_t   length;
} FData;

typedef struct inc_dir_s {
    uint8_t* path;
    size_t   raw_len;
    size_t   con_len;
    dev_t    dev;
    ino_t    ino;
} IncDir;

static FldErr dir_copy_path(const char* path, IncDir* dir);
static FldErr dir_get_fs_info(IncDir* dir);
static FldErr dir_is_duplicate(const IncDir* dir_list, const IncDir* dir);
static void   dir_find_concat_len(IncDir* dir);
static void   dir_free(IncDir* dir);

// clang-format off
//FARR_DEF_POP(IncDir, dir)
FARR_DEF_PUSH(IncDir, dir)

//FARR_DEF_POP(FData, fdata)
FARR_DEF_PUSH(FData, fdata)
// clang-format on

struct file_loader_s {
    FData*  files;
    IncDir* sys_dirs;
};

Fld_t* fld_new(void) {
    Fld_t* ldr = xmem_alloc(sizeof(Fld_t));
    if(ldr == NULL) {
        return NULL;
    }
    ldr->files = farr_new(FData, 64);
    if(ldr->files == NULL) {
        return NULL;
    }

    ldr->sys_dirs = farr_new(IncDir, 32);
    if(ldr->sys_dirs == NULL) {
        farr_free(FData, ldr->files);
        return NULL;
    }

    return ldr;
}

void fld_free(Fld_t* ldr) {
    if(ldr == NULL) {
        return;
    }
    if(ldr->files != NULL) {
        size_t cnt = farr_len(ldr->files);
        for(size_t i = 0; i < cnt; i++) {
            uint8_t* data = ldr->files[i].data;
            if(data != NULL) {
                xmem_free(data);
            }
        }
        farr_free(FData, ldr->files);
    }

    if(ldr->sys_dirs != NULL) {
        size_t cnt = farr_len(ldr->sys_dirs);
        for(size_t i = 0; i < cnt; i++) {
            dir_free(&(ldr->sys_dirs[i]));
        }
        farr_free(IncDir, ldr->sys_dirs);
    }

    xmem_free(ldr);
}

static FldErr fld_init_dir(IncDir* dir, const char* path) {
    FldErr err = dir_copy_path(path, dir);
    if(err != FLD_SUCCESS) {
        return err;
    }
    dir_find_concat_len(dir);

    err = dir_get_fs_info(dir);
    if(err != FLD_SUCCESS) {
        dir_free(dir);
        return err;
    }

    return FLD_SUCCESS;
}

FldErr fld_add_sys_dir(Fld_t* ldr, const char* path) {
    IncDir dir = { 0 };
    FldErr err = fld_init_dir(&dir, path);
    if(err != FLD_SUCCESS) {
        return err;
    }

    err = dir_is_duplicate(ldr->sys_dirs, &dir);
    if(err != FLD_SUCCESS) {
        dir_free(&dir);
        return err;
    }

    IncDir* tmp = ldr->sys_dirs;

    tmp = farr_push_dir(tmp, dir);
    if(tmp == NULL) {
        dir_free(&dir);
        return FLD_NO_MEM;
    }
    ldr->sys_dirs = tmp;

    return FLD_SUCCESS;
}

FldErr fld_add_user_dir(Fld_t ldr, const char* path) {
    IncDir dir = { 0 };
    FldErr err = fld_init_dir(&dir, path);
    if(err != FLD_SUCCESS) {
        return err;
    }
    // TODO
    return FLD_SUCCESS;
}

static FldErr dir_copy_path(const char* path, IncDir* dir) {
    if(path == NULL) {
        return FLD_INVALID_ARG;
    }
    size_t len = strlen(path);
    if(len == 0) {
        len  = 1;
        path = ".";
    }
    uint8_t* mem = xmem_alloc(len + 1);
    if(mem == NULL) {
        return FLD_NO_MEM;
    }
    memcpy(mem, path, len + 1);
    dir->path    = mem;
    dir->raw_len = len;
    return FLD_SUCCESS;
}

static void dir_free(IncDir* dir) {
    if(dir == NULL) {
        return;
    }
    if(dir->path != NULL) {
        xmem_free(dir->path);
    }
    memset(dir, 0, sizeof(IncDir));
}

static void dir_find_concat_len(IncDir* dir) {
    size_t   len  = dir->raw_len;
    uint8_t* path = dir->path;
    // I only care about trailing slashes.
    while(len > 1 && path[len - 1] == '/') {
        len -= 1;
    }
    dir->con_len = len;
}

static FldErr dir_get_fs_info(IncDir* dir) {
    // Larger than linuxs PATH_MAX so should be large enough
    // for most any path out there.
    char   buff[8196];
    size_t len = dir->con_len;
    if(len > (sizeof(buff) - 1)) {
        return FLD_PATH_TOO_LONG;
    }
    memcpy(buff, dir->path, len);
    buff[len] = '\0';

    struct stat st = { 0 };
    if(stat(buff, &st) < 0) {
        int err = errno;
        switch(err) {
            case EACCES:
                return FLD_PERM_DENIED;
            case ENOENT:
                return FLD_NON_EXISTENT;
            case ELOOP:
            case ENOTDIR:
                return FLD_PATH_NOT_WALKABLE;
            case ENAMETOOLONG:
                return FLD_PATH_TOO_LONG;
            case ENOMEM:
                return FLD_NO_MEM;
            default:
                fprintf(stderr, "error: Failed to stat() '%s' Errno(%d)\n", buff, err);
                return FLD_SYS_ERR;
        }
    }
    if(S_ISDIR(st.st_mode) == 0) {
        return FLD_NON_DIR;
    }
    dir->dev = st.st_dev;
    dir->ino = st.st_ino;

    return FLD_SUCCESS;
}

static FldErr dir_is_duplicate(const IncDir* dir_list, const IncDir* dir) {
    dev_t dev = dir->dev;
    ino_t ino = dir->ino;
    // Normally this won't be zero, but something like a FUSE fs
    // or non-linux fs just could cause funny business.
    // So just always add the directory so it can still be used.
    // Even if it leads to duplicate entries and extra work as
    // a result.
    if(dir->ino == 0) {
        return FLD_SUCCESS;
    }

    // Search for duplicates
    size_t len = farr_len(dir_list);
    for(size_t i = 0; i < len; i++) {
        if(dev != dir_list[i].dev) {
            continue;
        }
        if(ino == dir_list[i].ino) {
            return FLD_DUP_DIR;
        }
    }

    return FLD_SUCCESS;
}
