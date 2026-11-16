#ifndef _CONFIG_FILE_H
#define _CONFIG_FILE_H

#include <stdint.h>
#include "verbose.h"

typedef struct con_file_s ConFile;

typedef enum : uint32_t {
    CF_K_LINK = 0,
} ConFileKind;

typedef enum : uint32_t {
    CF_SUCCESS = 0,
    CF_NOT_FOUND,   // File not found.
    CF_NON_FILE,    // Provided file path is not a regular file.
    CF_TOO_LONG,    // File path too long
    CF_NO_PERM,     // Permission denied to file.
    CF_UNWALKABLE,  // File path can't be walked (e.g. loops ect...)
    CF_NO_MEM,      // Could not allocate memory for the file
    CF_UNEXPEC_EOF, // While reading the file an EOF happened early.

    CF_CMD_DONE,    // No more lines to process
    CF_CMD_EMPTY,   // Line was empty.
    CF_CMD_UNK_KIND,// Unrecognized command kind.

    CF_SYS_ERROR,   // A system error of some kind occurred.
} ConFileErr;

typedef struct env_cmd_s {
    const uint8_t* src;
    const uint8_t* dst;
    ConFileKind    kind;
} ConFileCmd;

ConFileErr con_file_new (ConFile** new_file, const char* path);
void       con_file_free(ConFile* file);

// Returns NULL when there are no more lines.
const uint8_t *con_file_next_line(ConFile* cf);

ConFileErr con_file_next_cmd(ConFile* cf, ConFileCmd* cmd_out);

void con_file_print_new_error(ConFileErr err, const char* path);

#endif