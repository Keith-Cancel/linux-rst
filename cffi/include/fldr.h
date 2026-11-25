#ifndef FILE_LOADER_H
#define FILE_LOADER_H

typedef struct file_loader_s Fld_t;

typedef enum {
    FLD_SUCCESS = 0,
    FLD_NO_MEM,
    FLD_PERM_DENIED,
    FLD_NON_EXISTENT,

    FLD_DIR_DUPLICATE,
    FLD_DIR_SHADOWED,

    FLD_PATH_TOO_LONG,
    FLD_PATH_NOT_DIR,
    FLD_PATH_NOT_WALKABLE,

    FLD_SYS_ERR,
    FLD_INVALID_ARG,
} FldErr;

typedef enum {
    DIR_KIND_QUOTE = 0,
    DIR_KIND_INC,
    DIR_KIND_SYS,
    // Not a kind just the count
    DIR_COUNT_OF_KIND,
} FldDirKind_t;

// Info on what path that was early added
// got shadowed by the latest addition.
typedef struct fld_shadow_s {
    const char*  path;
    FldDirKind_t kind;
} FldShadow_t;


Fld_t* fld_new(void);
void   fld_free(Fld_t* ldr);

FldErr fld_add_quote_dir(Fld_t* ldr, const char* path, FldShadow_t* shadowed);
FldErr fld_add_inc_dir(Fld_t* ldr, const char* path, FldShadow_t* shadowed);
FldErr fld_add_sys_dir(Fld_t* ldr, const char* path, FldShadow_t* shadowed);

#endif
