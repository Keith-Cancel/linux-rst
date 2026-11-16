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
    CF_CMD_DONE,    // No more lines to process
    CF_CMD_EMPTY,   // Line was empty.
    CF_CMD_UNK_KIND,// Unrecognized command kind.
} ConFileErr;

typedef struct env_cmd_s {
    const uint8_t* src;
    const uint8_t* dst;
    ConFileKind    kind;
} ConFileCmd;

ConFile* con_file_new(const char* path, Verbose verbose);
void     con_file_free(ConFile* file);

// Returns NULL when there are no more lines.
const uint8_t *con_file_next_line(ConFile* cf);

ConFileErr con_file_next_cmd(ConFile* cf, ConFileCmd* cmd_out);

#endif