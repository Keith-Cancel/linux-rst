#include <stdint.h>
#include <stdlib.h>

#include "fldr.h"


typedef struct file_data_s {
    uint8_t* data;
    size_t   length;
} FData;

struct file_loader_s {
    FData* files;
    size_t count;
    size_t cap;
};

Fldr_t* fldr_new() {
    Fldr_t* ldr = malloc(sizeof(Fldr_t));
    if(ldr == NULL) {
        return NULL;
    }
    size_t cap = 64;

    FData* list = malloc(cap * sizeof(FData));
    if(list == NULL) {
        free(ldr);
        return NULL;
    }

    ldr->cap   = cap;
    ldr->files = list;
    ldr->count = 0;
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
    for(size_t i = 0; i < ldr->count; i++) {
        FData* file = (ldr->files + i);
        if(file->data != NULL) {
            free(file->data);
        }
    }
    free(ldr->files);
    free(ldr);
}
