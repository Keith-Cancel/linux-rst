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

#define DIR_IS_QUO 0
#define DIR_IS_INC 1
#define DIR_IS_SYS 2

#define DIR_KIND_CNT 3

typedef struct fs_info_s {
    dev_t dev;
    ino_t ino;
} FsInfo;

typedef struct file_data_s {
    uint8_t* data;
    size_t   length;
} FData;

typedef struct inc_dir_s {
    char*    path;
    FsInfo   fs;
    uint16_t raw_len;
    uint16_t con_len;
    bool     shadowed;
} IncDir;

static FldErr  dir_init(IncDir* dir, const char* path);
static ssize_t dir_dup_index(const IncDir* dir, const IncDir* dir_list, size_t len);
static void    dir_free(IncDir* dir);

static FldErr fld_add_dir(Fld_t* ldr, const char* path, unsigned kind);
static void   fld_free_dir_list(IncDir* list);
static bool   fld_is_duplicate(const Fld_t* ldr, const IncDir* dir, unsigned kind);
static bool   fld_shadow_duplicate(const Fld_t* ldr, const IncDir* dir, unsigned kind);

static size_t path_concat_len(const char* path, size_t len);
static FldErr path_get_fs_info(const char* path, size_t len, FsInfo* info);

// clang-format off
//FARR_DEF_POP(IncDir, dir)
FARR_DEF_PUSH(IncDir, dir)
//FARR_DEF_LAST(IncDir, dir)

//FARR_DEF_POP(FData, fdata)
//FARR_DEF_PUSH(FData, fdata)
// clang-format on

struct file_loader_s {
    FData*  files;
    IncDir* dirs[3];
};

// ========== File Loader Public ==========

Fld_t* fld_new(void) {
    Fld_t* ldr = xmem_alloc(sizeof(Fld_t));
    if(ldr == NULL) {
        return NULL;
    }
    memset(ldr, 0, sizeof(Fld_t));

    ldr->files = farr_new(FData, 32);
    if(ldr->files == NULL) {
        xmem_free(ldr);
        return NULL;
    }

    for(int i = 0; i < DIR_KIND_CNT; i++) {
        ldr->dirs[i] = farr_new(IncDir, 32);
        if(ldr->dirs[i] == NULL) {
            fld_free(ldr);
            return NULL;
        }
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

    for(unsigned i = 0; i < DIR_KIND_CNT; i++) {
        fld_free_dir_list(ldr->dirs[i]);
    }

    xmem_free(ldr);
}

FldErr fld_add_quote_dir(Fld_t* ldr, const char* path) {
    return fld_add_dir(ldr, path, DIR_IS_QUO);
}

FldErr fld_add_inc_dir(Fld_t* ldr, const char* path) {
    return fld_add_dir(ldr, path, DIR_IS_INC);
}

FldErr fld_add_sys_dir(Fld_t* ldr, const char* path) {
    return fld_add_dir(ldr, path, DIR_IS_SYS);
}

// ========= File Loader Functions ========

static void fld_free_dir_list(IncDir* list) {
    if(list == NULL) {
        return;
    }
    size_t cnt = farr_len(list);
    for(size_t i = 0; i < cnt; i++) {
        dir_free(&(list[i]));
    }
    farr_free(IncDir, list);
}

// Checks for duplicate directories at kind, and kinds of higher value.
static bool fld_is_duplicate(const Fld_t* ldr, const IncDir* dir, unsigned kind) {
    for(unsigned i = kind; i < DIR_KIND_CNT; i++) {
        size_t len = farr_len(ldr->dirs[i]);
        if(dir_dup_index(dir, ldr->dirs[i], len) >= 0) {
            return true;
        }
    }
    return false;
}

// Checks for duplicate directories for kinds with a lower value than current kind.
// If it finds any it sets them to be shadowed.
static bool fld_shadow_duplicate(const Fld_t* ldr, const IncDir* dir, unsigned kind) {
    bool shadowed = false;
    for(unsigned i = 0; i < kind; i++) {
        size_t  len = farr_len(ldr->dirs[i]);
        ssize_t idx = dir_dup_index(dir, ldr->dirs[i], len);
        if(idx < 0) {
            continue;
        }
        shadowed                   = true;
        ldr->dirs[i][idx].shadowed = true;
    }
    return shadowed;
}

static FldErr fld_add_dir(Fld_t* ldr, const char* path, unsigned kind) {
    if(kind >= DIR_KIND_CNT) {
        return FLD_INVALID_ARG;
    }

    IncDir dir = { 0 };
    FldErr err = dir_init(&dir, path);
    if(err != FLD_SUCCESS) {
        return err;
    }

    // Check if it's a dup in any lower level lists.
    if(fld_is_duplicate(ldr, &dir, kind)) {
        dir_free(&dir);
        return FLD_DIR_DUPLICATE;
    }

    IncDir* ptr = farr_push_dir(ldr->dirs[kind], dir);
    if(ptr == NULL) {
        dir_free(&dir);
        return FLD_NO_MEM;
    }
    ldr->dirs[kind] = ptr;


    if(fld_shadow_duplicate(ldr, &dir, kind)) {
        return FLD_DIR_SHADOW;
    }
    return FLD_SUCCESS;
}

// ========== Directory Functions =========

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

static ssize_t dir_dup_index(const IncDir* dir, const IncDir* dir_list, size_t len) {
    dev_t dev = dir->fs.dev;
    ino_t ino = dir->fs.ino;
    // Normally this won't be zero, but something like a FUSE fs
    // or non-linux fs just could cause funny business.
    // So just always add the directory so it can still be used.
    // Even if it leads to duplicate entries and extra work as
    // a result.
    if(ino == 0) {
        return -1;
    }
    // Search for duplicates
    for(size_t i = 0; i < len; i++) {
        ino_t cur_ino = dir_list[i].fs.ino;

        if(dir_list[i].shadowed || cur_ino == 0) {
            continue;
        }
        if(dev != dir_list[i].fs.dev) {
            continue;
        }
        if(ino == cur_ino) {
            return i;
        }
    }
    return -1;
}

// ============ Path Functions ============

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
        return FLD_PATH_NOT_DIR;
    }
    info->dev = st.st_dev;
    info->ino = st.st_ino;

    return FLD_SUCCESS;
}
