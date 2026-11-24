#include <stdint.h>
#include <stdlib.h>

#include "fldr.h"
#include "flex_arr.h"

typedef struct file_data_s {
    uint8_t* data;
    size_t   length;
} FData;

// clang-format off
FARR_DEF_POP(FData, fdata)
FARR_DEF_PUSH(FData, fdata)
// clang-format on

struct file_loader_s {
    FData* files;
};

Fldr_t* fldr_new() {
    Fldr_t* ldr = malloc(sizeof(Fldr_t));
    if(ldr == NULL) {
        return NULL;
    }
    ldr->files = farr_new(FData, 64);
    if(ldr->files == NULL) {
        return NULL;
    }
    return ldr;
}

void fldr_free(Fldr_t* ldr) {
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
            free(data);
        }
    }
    farr_free(FData, ldr->files);
    free(ldr);
}
