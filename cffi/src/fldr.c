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
    size_t   len;
} IncDir;

static FldErr copy_path(const char* path, IncDir* dir);

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
    dir->path = mem;
    dir->len  = len;
    return FLD_SUCCESS;
}
