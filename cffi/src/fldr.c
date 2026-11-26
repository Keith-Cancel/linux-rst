#include <errno.h>
#include <stdio.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "types.h"
#include "fldr.h"
#include "flex_arr.h"
#include "util.h"
#include "xmem.h"

typedef struct fs_info_s {
    dev_t dev;
    ino_t ino;
} FsInfo;

typedef struct file_data_s {
    u8* data;
    usz length;
} FData;

typedef struct inc_dir_s {
    char*  path;
    FsInfo fs;
    u16    raw_len;
    u16    con_len;
    bool   shadowed;
} IncDir;

static FldErr dir_init(IncDir* dir, const char* path);
static isz    dir_dup_index(const IncDir* dir, const IncDir* dir_list, usz len);
static void   dir_free(IncDir* dir);

static void   fld_free_dir_list(IncDir* list);
static bool   fld_is_duplicate(const Fld_t* ldr, const IncDir* dir, int kind);
static FldErr fld_add_dir(Fld_t* ldr, const char* path, int kind, FldShadow_t* shadowed);
static bool   fld_shadow_duplicate(const Fld_t* ldr, const IncDir* dir, int kind, FldShadow_t* shadowed);

static usz    path_rtrim_sep_len(const char* path, usz len);
static bool   path_is_absolute(const char* path, usz len);
static FldErr path_get_fs_info(const char* path, usz len, FsInfo* info);
static bool   path_join_raw(char* buf, usz blen, const char* dir, usz dlen, const char* path, usz plen);

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
        usz cnt = farr_len(ldr->files);
        for(usz i = 0; i < cnt; i++) {
            u8* data = ldr->files[i].data;
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

void fld_lookup_and_load_bracket(Fld_t* ldr, const char* path, usz path_len) {
    if(path_is_absolute(path, path_len)) {}

    Buff4k_t buf;
    for(int k = DIR_KIND_INC; k < DIR_COUNT_OF_KIND; k++) {
        IncDir* list = ldr->dirs[k];
        usz     len  = farr_len(list);
        for(usz i = 0; i < len; i++) {
            IncDir* d = &(list[i]);
            if(d->shadowed) {
                continue;
            }
            if(!path_join_raw(buf.char_c, sizeof(buf), d->path, d->con_len, path, path_len)) {
                continue;
            }
        }
    }
}

// ========= File Loader Functions ========

static void fld_free_dir_list(IncDir* list) {
    if(list == NULL) {
        return;
    }
    usz cnt = farr_len(list);
    for(usz i = 0; i < cnt; i++) {
        dir_free(&(list[i]));
    }
    farr_free(IncDir, list);
}

// Checks for duplicate directories at kind, and kinds of higher value.
static bool fld_is_duplicate(const Fld_t* ldr, const IncDir* dir, int kind) {
    for(int i = kind; i < DIR_COUNT_OF_KIND; i++) {
        usz len = farr_len(ldr->dirs[i]);
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
        usz len = farr_len(ldr->dirs[i]);
        isz idx = dir_dup_index(dir, ldr->dirs[i], len);
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
    usz plen = strlen(path);
    if(plen == 0) {
        path = ".";
        plen = 1;
    }
    if(plen > (sizeof(Buff4k_t) - 1)) {
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

    usz   bytes = dir->raw_len + 1;
    char* mem   = xmem_alloc(bytes);
    if(mem == NULL) {
        return FLD_NO_MEM;
    }
    imem_copy(mem, path, bytes);
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

static isz dir_dup_index(const IncDir* dir, const IncDir* dir_list, usz len) {
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
    for(usz i = 0; i < len; i++) {
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
static usz path_rtrim_sep_len(const char* path, usz len) {
    // The case of a bare `//` thoughts:
    // POSIX says just `//` at the start of a path is implementation
    // defined. However, if I think about concatenating later
    // `//` + `foo/bar.h` for something like #include <foo/bar.h>.
    // The only obvious thing I could think of is a network share
    // with a host of `foo`, and a share of `bar.h`. That does not
    // really seem sane. I probably, just want root. Some bizarre
    // FUSE fs could do be something more sane, but still.
    // So I don't think I want to special case just `//`.

    // I only care about trailing slashes.
    while(len > 1 && path[len - 1] == '/') {
        len -= 1;
    }
    return len;
}

static bool path_is_absolute(const char* path, usz len) {
    if(len == 0 || path == NULL) {
        return false;
    }
    return path[0] == '/';
}

// Joins `dir` + SEP + `path` + `\0` into `buf`.
// Performs no path normalization or duplicate separator stripping.
// Returns true on success, or false if the buffer is too small.
static bool path_join_raw(char* buf, usz blen, const char* dir, usz dlen, const char* path, usz plen) {
    // Buffer too small
    if(blen < 2 || dlen > (blen - 2) || plen > (blen - 2 - dlen)) {
        return false;
    }

    buf  = imem_copy(buf, dir, dlen);
    *buf = '/';
    buf  = imem_copy(buf + 1, path, plen);
    *buf = '\0';
    return true;
}

static FldErr path_get_fs_info(const char* path, usz len, FsInfo* info) {
    // Larger than linuxs PATH_MAX so should be large enough
    // for most any path out there.
    Buff4k_t buff;
    if(len > (sizeof(buff) - 1)) {
        return FLD_PATH_TOO_LONG;
    }
    *((char*)imem_copy(buff.char_c, path, len)) = '\0';

    struct stat st = { 0 };
    if(stat(buff.char_c, &st) < 0) {
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
                fprintf(stderr, "error: Failed to stat() '%s' Errno(%d)\n", buff.char_c, err);
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
