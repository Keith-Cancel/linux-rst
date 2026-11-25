#ifndef FILE_LOADER_H
#define FILE_LOADER_H

typedef struct file_loader_s Fld_t;

typedef enum {
    FLD_SUCCESS = 0,
    FLD_NO_MEM,
    FLD_PERM_DENIED,
    FLD_NON_EXISTENT,
    FLD_NON_DIR,
    FLD_DUP_DIR,
    FLD_IGNORE_DIR,
    FLD_PATH_TOO_LONG,
    FLD_PATH_NOT_WALKABLE,
    FLD_SYS_ERR,
    FLD_INVALID_ARG,
} FldErr;

Fld_t* fld_new(void);
void   fld_free(Fld_t* ldr);

FldErr fld_add_quote_dir(Fld_t* ldr, const char* path);
FldErr fld_add_inc_dir(Fld_t* ldr, const char* path);
FldErr fld_add_sys_dir(Fld_t* ldr, const char* path);

#endif
