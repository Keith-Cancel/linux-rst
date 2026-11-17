#ifndef _CONFIG_FILE_H
#define _CONFIG_FILE_H

#include <stdint.h>

typedef struct con_file_s ConFile;

typedef enum : uint32_t {
    CF_K_NONE = 0,
    CF_K_LINK,
} ConFileKind;

#define CF_E_FILE(v) (((uint32_t)0x0000'0000) | (uint16_t)(v))
#define CF_E_CMD(v)  (((uint32_t)0x0001'0000) | (uint16_t)(v))

#define CF_E_CMD_S(g, v) CF_E_CMD((((uint16_t)(g)) << 8) | (uint8_t)(v))

// The second byte determines the kinda of error
typedef enum : uint32_t {
    CF_SUCCESS     = CF_E_FILE(0),
    CF_NOT_FOUND   = CF_E_FILE(1), // File not found.
    CF_NON_FILE    = CF_E_FILE(2), // Provided file path is not a regular file.
    CF_TOO_LONG    = CF_E_FILE(3), // File path too long
    CF_NO_PERM     = CF_E_FILE(4), // Permission denied to file.
    CF_UNWALKABLE  = CF_E_FILE(5), // File path can't be walked (e.g. loops etc...)
    CF_NO_MEM      = CF_E_FILE(6), // Could not allocate memory for the file
    CF_UNEXPEC_EOF = CF_E_FILE(7), // While reading the file an EOF happened early.
    // A system error of some kind occurred.
    CF_SYS_ERROR   = CF_E_FILE(0xffff), 

    // For cmd errors
    // the high byte is the group
    // the low  byte is the error for that group.
    // each cmd is of the form  <kind> <sp> " <dst> " <sp> " <src> "
    CF_CMD_DONE         = CF_E_CMD_S(0, 0), // No more lines to process
    CF_CMD_EMPTY        = CF_E_CMD_S(0, 1), // The command's line was empty.
    CF_CMD_TRAILING     = CF_E_CMD_S(0, 2), // Trailing data after <src> close `"`
    CF_CMD_KIND_UNKNOWN = CF_E_CMD_S(0, 3), // The line's command <kind> is unrecognized.

    // DST and SRC will be the exact same except the high nibble.
    CF_CMD_DST_UNOPENED = CF_E_CMD_S(1, 0), // The line's <dst> is missing an opening `"`
    CF_CMD_DST_UNCLOSED = CF_E_CMD_S(1, 1), // The line's <dst> is missing a closing `"`
    CF_CMD_DST_BAD_ESC  = CF_E_CMD_S(1, 2), // The line's <src> has a bad or unrecognized escape.
    CF_CMD_DST_NO_SP    = CF_E_CMD_S(1, 3), // The is no space character before the <src>
    
    CF_CMD_SRC_UNOPENED = CF_E_CMD_S(2, 0), // The line's <src> is missing an opening `"`
    CF_CMD_SRC_UNCLOSED = CF_E_CMD_S(2, 1), // The line's <src> is missing a closing `"`
    CF_CMD_SRC_BAD_ESC  = CF_E_CMD_S(2, 2), // The line's <src> has a bad or unrecognized escape.
    CF_CMD_SRC_NO_SP    = CF_E_CMD_S(2, 3), // The is no space character before the <src>
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