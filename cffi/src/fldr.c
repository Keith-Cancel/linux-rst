#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fldr.h"
#include "flex_arr.h"
#include "xmem.h"

typedef struct file_data_s {
    uint8_t* data;
    size_t   length;
} FData;

typedef struct inc_dir_s {
    uint8_t* path;
    size_t   full_len;
    size_t   concat_len;
} IncDir;

static FldErr copy_path(const char* path, IncDir* dir);
static void   dir_find_concat_len(IncDir* dir);

// clang-format off
FARR_DEF_POP(FData, fdata)
FARR_DEF_PUSH(FData, fdata)
// clang-format on

struct file_loader_s {
    FData* files;
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
    return ldr;
}

void fld_free(Fld_t* ldr) {
    if(ldr == NULL) {
        return;
    }
    if(ldr->files == NULL) {
        free(ldr);
        return;
    }
    size_t cnt = farr_len(ldr->files);
    for(size_t i = 0; i < cnt; i++) {
        uint8_t* data = ldr->files[i].data;
        if(data != NULL) {
            xmem_free(data);
        }
    }
    farr_free(FData, ldr->files);
    xmem_free(ldr);
}

FldErr fld_add_sys_path(const char* path) {
    IncDir dir = { 0 };
    FldErr err = copy_path(path, &dir);
    if(err != FLD_SUCCESS) {
        return err;
    }
    dir_find_concat_len(&dir);


    return FLD_SUCCESS;
}

static FldErr copy_path(const char* path, IncDir* dir) {
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
    dir->path     = mem;
    dir->full_len = len;
    return FLD_SUCCESS;
}

static void dir_find_concat_len(IncDir* dir) {
    size_t   len  = dir->full_len;
    uint8_t* path = dir->path;
    // I only care about trailing slashes.
    while(len > 1 && path[len - 1] == '/') {
        len -= 1;
    }
    dir->concat_len = len;
}
