#ifndef _CONFIG_FILE_H
#define _CONFIG_FILE_H

#include <stdint.h>
#include "verbose.h"

typedef struct con_file_s ConFile;

typedef enum : uint32_t {
    CMD_LINK = 0,
} CmdKind;

typedef struct env_cmd_s {
    const uint8_t* src;
    const uint8_t* dst;
    CmdKind        kind;
} EnvCmd;

ConFile* con_file_new(const char* path, Verbose verbose);
void     con_file_free(ConFile* file);

// Returns NULL when there are no more lines.
const uint8_t *con_file_next_line(ConFile* cf);

bool con_file_next_cmd(ConFile* cf, EnvCmd* cmd_out);

#endif