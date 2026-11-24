#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "fldr.h"
#include "flex_arr.h"
#include "xmem.h"

#define PATH_BUFF 8192

typedef struct file_data_s {
    uint8_t* data;
    size_t   length;
} FData;

typedef struct fs_info_s {
    dev_t dev;
    ino_t ino;
} FsInfo;

typedef struct inc_dir_s {
    char*    path;
    FsInfo   fs;
    uint16_t raw_len;
    uint16_t con_len;
    bool     ignore;
} IncDir;

static FldErr dir_init(IncDir* dir, const char* path);
static FldErr dir_is_duplicate(const IncDir* dir, const IncDir* dir_list, size_t len);
static void   dir_free(IncDir* dir);

static size_t path_concat_len(const char* path, size_t len);
static FldErr path_get_fs_info(const char* path, size_t len, FsInfo* info);

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

FldErr fld_add_sys_dir(Fld_t* ldr, const char* path) {
    IncDir dir = { 0 };
    FldErr err = dir_init(&dir, path);
    if(err != FLD_SUCCESS) {
        return err;
    }

    err = dir_is_duplicate(&dir, ldr->sys_dirs, farr_len(ldr->sys_dirs));
    if(err != FLD_SUCCESS) {
        dir_free(&dir);
        return err;
    }

    IncDir* tmp = ldr->sys_dirs;
    tmp         = farr_push_dir(tmp, dir);
    if(tmp == NULL) {
        dir_free(&dir);
        return FLD_NO_MEM;
    }
    ldr->sys_dirs = tmp;

    return FLD_SUCCESS;
}

FldErr fld_add_user_dir(Fld_t* ldr, const char* path) {
    IncDir dir = { 0 };
    FldErr err = dir_init(&dir, path);
    if(err != FLD_SUCCESS) {
        return err;
    }
    // TODO
    return FLD_SUCCESS;
}

static FldErr dir_init(IncDir* dir, const char* path) {
    if(path == NULL) {
        return FLD_INVALID_ARG;
    }
    size_t plen = strlen(path);
    if(plen == 0) {
        path = ".";
        plen = 1;
    }
    if(plen > (PATH_BUFF - 1)) {
        return FLD_PATH_TOO_LONG;
    }

    dir->raw_len = plen;
    dir->con_len = path_concat_len(path, plen);

    FsInfo info = { 0 };
    FldErr err  = path_get_fs_info(path, dir->con_len, &info);
    if(err != FLD_SUCCESS) {
        return err;
    }
    dir->fs = info;

    size_t bytes = dir->raw_len + 1;
    char*  mem   = xmem_alloc(bytes);
    if(mem == NULL) {
        return FLD_NO_MEM;
    }
    memcpy(mem, path, bytes);
    dir->path = mem;
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

static FldErr dir_is_duplicate(const IncDir* dir, const IncDir* dir_list, size_t len) {
    dev_t dev = dir->fs.dev;
    ino_t ino = dir->fs.ino;
    // Normally this won't be zero, but something like a FUSE fs
    // or non-linux fs just could cause funny business.
    // So just always add the directory so it can still be used.
    // Even if it leads to duplicate entries and extra work as
    // a result.
    if(ino == 0) {
        return FLD_SUCCESS;
    }
    // Search for duplicates
    for(size_t i = 0; i < len; i++) {
        if(dev != dir_list[i].fs.dev) {
            continue;
        }
        if(ino == dir_list[i].fs.ino) {
            return FLD_DUP_DIR;
        }
    }
    return FLD_SUCCESS;
}

static size_t path_concat_len(const char* path, size_t len) {
    // I only care about trailing slashes.
    while(len > 1 && path[len - 1] == '/') {
        len -= 1;
    }
    return len;
}

static FldErr path_get_fs_info(const char* path, size_t len, FsInfo* info) {
    // Larger than linuxs PATH_MAX so should be large enough
    // for most any path out there.
    char buff[PATH_BUFF];
    if(len > (sizeof(buff) - 1)) {
        return FLD_PATH_TOO_LONG;
    }
    memcpy(buff, path, len);
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
    info->dev = st.st_dev;
    info->ino = st.st_ino;

    return FLD_SUCCESS;
}
