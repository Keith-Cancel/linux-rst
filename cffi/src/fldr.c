#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "fldr.h"
#include "flex_arr.h"
#include "xmem.h"

#define PATH_BUFF 4096

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

static void   fld_free_dir_list(IncDir* list);
static bool   fld_is_duplicate(const Fld_t* ldr, const IncDir* dir, int kind);
static FldErr fld_add_dir(Fld_t* ldr, const char* path, int kind, FldShadow_t* shadowed);
static bool   fld_shadow_duplicate(const Fld_t* ldr, const IncDir* dir, int kind, FldShadow_t* shadowed);

static size_t path_rtrim_sep_len(const char* path, size_t len);
static bool   path_is_absolute(const char* path);
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

    for(int i = 0; i < DIR_COUNT_OF_KIND; i++) {
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

    for(int i = 0; i < DIR_COUNT_OF_KIND; i++) {
        fld_free_dir_list(ldr->dirs[i]);
    }

    xmem_free(ldr);
}

FldErr fld_add_quote_dir(Fld_t* ldr, const char* path, FldShadow_t* shadowed) {
    return fld_add_dir(ldr, path, DIR_KIND_QUOTE, shadowed);
}

FldErr fld_add_inc_dir(Fld_t* ldr, const char* path, FldShadow_t* shadowed) {
    return fld_add_dir(ldr, path, DIR_KIND_INC, shadowed);
}

FldErr fld_add_sys_dir(Fld_t* ldr, const char* path, FldShadow_t* shadowed) {
    return fld_add_dir(ldr, path, DIR_KIND_SYS, shadowed);
}

void fld_lookup_and_load_quote(Fld_t* ldr, const char* path) {
}

void fld_lookup_and_load_bracket(Fld_t* ldr, const char* path) {
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
static bool fld_is_duplicate(const Fld_t* ldr, const IncDir* dir, int kind) {
    for(int i = kind; i < DIR_COUNT_OF_KIND; i++) {
        size_t len = farr_len(ldr->dirs[i]);
        if(dir_dup_index(dir, ldr->dirs[i], len) >= 0) {
            return true;
        }
    }
    return false;
}

// Checks for duplicate directories for kinds with a lower value than current kind.
// If it finds any it sets them to be shadowed.
static bool fld_shadow_duplicate(const Fld_t* ldr, const IncDir* dir, int kind, FldShadow_t* info) {
    bool shadowed = false;
    for(int i = 0; i < kind; i++) {
        size_t  len = farr_len(ldr->dirs[i]);
        ssize_t idx = dir_dup_index(dir, ldr->dirs[i], len);
        if(idx < 0) {
            continue;
        }
        IncDir* dup = &(ldr->dirs[i][idx]);
        if(info != NULL) {
            info->kind = i;
            info->path = dup->path;
        }
        shadowed      = true;
        dup->shadowed = true;
        // Probably could just return I don't think
        // any other are possible because I check
        // on every add directory for duplicates/shadows.
    }
    return shadowed;
}

static FldErr fld_add_dir(Fld_t* ldr, const char* path, int kind, FldShadow_t* shadowed) {
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


    if(fld_shadow_duplicate(ldr, &dir, kind, shadowed)) {
        return FLD_DIR_SHADOWED;
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
    dir->con_len = path_rtrim_sep_len(path, plen);

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
    dir->path     = mem;
    dir->shadowed = false;
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
// Currently, these functions only handle unix style paths. I have not
// bothered with windows yet.

// Returns the length of `path` with trailing path separators removed.
// It will always retain the root/drive etc...
// So it will never be below 1 so like unix a root path stays "/".
static size_t path_rtrim_sep_len(const char* path, size_t len) {
    // Thase of just `//` thoughts.
    // POSIX says just `//` at the start of a path is implementation
    // defined. However, if I think about concatenating later
    // `//` + `foo/bar.h` for something like #include <foo/bar.h>.
    // The only thing I think that could be is like a network share
    // with a host of `foo`, and a share of `bar.h`. That does not
    // really seem sane. I probably, just want root. Some bizarre
    // FUSE fs could be be something more sane, but still.
    // So I don't think I want to special case just `//`.

    // I only care about trailing slashes.
    while(len > 1 && path[len - 1] == '/') {
        len -= 1;
    }
    return len;
}

static bool path_is_absolute(const char* path) {
    if(path == NULL) {
        return false;
    }
    return path[0] == '/';
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
