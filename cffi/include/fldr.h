#ifndef FILE_LOADER_H
#define FILE_LOADER_H

typedef struct file_loader_s Fld_t;

typedef enum {
    FLD_SUCCESS = 0,
    FLD_NO_MEM,
    FLD_INVALID_ARG,
} FldErr;

Fld_t* fld_new(void);
void   fld_free(Fld_t* ldr);

#endif
