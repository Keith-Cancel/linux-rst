#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbit.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "conf.h"
#include "diag.h"
#include "utf8.h"


static uint8_t*   next_line_priv(ConFile *cf);
static ConFileErr cf_map_errno(int errnum);
static bool       map_kind(const uint8_t* kind, ConFileKind* out);

static bool       is_comment(uint8_t* str);
static uint8_t    parse_quote_str(uint8_t* str, uint8_t** after);

#define Q_UNOPENED 0     // missing opening `"`
#define Q_UNCLOSED 1     // missing a closing `"`
#define Q_BAD_ESC  2     // bad escape sequence
#define Q_SUCCESS  0xff

struct con_file_s {
    uint8_t* data;
    size_t   cursor;
    size_t   size;
};

ConFileErr con_file_new(ConFile** new_file, const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if(fd < 0) {
        return cf_map_errno(errno);
    }

    struct stat st = { 0 };
    if(fstat(fd, &st) < 0) {
        D_ERROR("fstat() for the config file failed", errno);
        close(fd);
        return CF_SYS_ERROR;
    }

    if(!S_ISREG(st.st_mode)) {
        close(fd);
        return CF_NON_FILE;
    }

    ConFile *cf = malloc(sizeof(ConFile));
    if (cf == nullptr) {
        close(fd);
        return CF_NO_MEM;
    }

    cf->size   = (size_t)st.st_size;
    cf->cursor = 0;
    // Over allocate by 4
    cf->data   = malloc(cf->size + 4);
    if(cf->data == nullptr) {
        free(cf);
        close(fd);
        return CF_NO_MEM;
    }
    // Zero the over allocation
    memset((cf-> data + cf->size), 0, 4);

    size_t   rem = cf->size;
    bool     eof = false;
    uint8_t* pos = cf->data;
    while(rem > 0) {
        ssize_t amt = read(fd, pos, rem);
        if(amt < 0) {
            if(errno == EINTR) {
                continue;
            }
            break;
        }
        if(amt == 0) {
            eof = true;
            break;
        }
        pos += amt;
        rem -= amt;
    }
    int errnum = errno;
    close(fd);

    if(rem > 0 && eof) {
        con_file_free(cf);
        return CF_UNEXPEC_EOF;
    }
    if(rem > 0) {
        con_file_free(cf);
        D_ERROR("Failed reading config file", errnum);
        return CF_SYS_ERROR;
    }
    *new_file = cf;
    return CF_SUCCESS;
}

// Returns NULL when there are no more lines.
const uint8_t *con_file_next_line(ConFile *cf) {
    return next_line_priv(cf);
}

ConFileErr con_file_next_cmd(ConFile* cf, ConFileCmd* cmd_out) {
    cmd_out->kind = CF_K_NONE;
    cmd_out->src  = (uint8_t*)"";
    cmd_out->dst  = (uint8_t*)"";
    uint8_t* ln = next_line_priv(cf);
    if(ln == nullptr) {
        return CF_CMD_DONE;
    }
    if(ln[0] == '\0') {
        return CF_CMD_EMPTY;
    }
    // Line Comment
    if(is_comment(ln)) {
        return CF_CMD_EMPTY;
    }

    uint8_t* kind = ln;
    uint8_t* dst  = nullptr;
    uint8_t* src  = nullptr;

    uint8_t* pos  = ln;
    // Find the first space or end of str
    while(*pos != '\0' && *pos != ' ') {
        pos += 1;
    }
    // A space must exist before <dst>.
    if(*pos != ' ') {
        return CF_CMD_DST_NO_SP;
    }
    // Overwrite the space with '\0' to make a new string for <kind>
    *pos = '\0';
    pos += 1;

    dst = pos;
    uint8_t ret = parse_quote_str(pos, &pos);
    if(ret != Q_SUCCESS) {
        return CF_E_CMD_S(1, ret);
    }
    cmd_out->dst = dst;

    // A space must exist before <src>
    if(*pos != ' ') {
        return CF_CMD_SRC_NO_SP;
    }
    pos += 1;

    src = pos;
    ret = parse_quote_str(pos, &pos);
    if(ret != Q_SUCCESS) {
        return CF_E_CMD_S(2, ret);
    }
    cmd_out->src = src;

    // No extra data after closing quote of <src>.
    if(*pos != '\0') {
        return CF_CMD_TRAILING;
    }

    if(!map_kind(kind, &(cmd_out->kind))) {
        return CF_CMD_KIND_UNKNOWN;
    }
    return CF_SUCCESS;
}

static bool map_kind(const uint8_t* kind, ConFileKind* out) {
    if(strcmp((const char*)kind, "lnk") == 0) {
        *out = CF_K_LINK;
        return true;
    }
    if(strcmp((const char*)kind, "bindf") == 0) {
        *out = CF_K_BINDF;
        return true;
    }
    if(strcmp((const char*)kind, "bindf_rw") == 0) {
        *out = CF_K_BINDF_RW;
        return true;
    }
    if(strcmp((const char*)kind, "bindd") == 0) {
        *out = CF_K_BINDD;
        return true;
    }
    if(strcmp((const char*)kind, "bindd_rw") == 0) {
        *out = CF_K_BINDD_RW;
        return true;
    }
    return false;
}

void con_file_free(ConFile* file) {
    if(file == nullptr) {
        return;
    }
    if(file->data != nullptr) {
        free(file->data);
    }
    free(file);
}

void con_file_print_new_error(ConFileErr err, const char* path) {
    const char* msg = "UNKNOWN";
    switch(err) {
        case CF_NOT_FOUND:
            msg = "Config file not found";
            break;
        case CF_NON_FILE:
            msg = "Config file is not a regular file";
            break;
        case CF_TOO_LONG:
            msg = "Config file path is too long";
            break;
        case CF_NO_PERM:
            msg = "Permission denied for config file";
            break;
        case CF_UNWALKABLE:
            msg = "A path component of the config file is not walkable";
            break;
        case CF_SYS_ERROR:
            msg = "A system error occurred";
            break;
        default:
            D_ERROR("Unknown config error");
            break;
    }
    fprintf(stderr, "Error: %s: '%s'\n", msg, path);
}

static ConFileErr cf_map_errno(int errnum) {
    switch(errnum) {
        case ENOENT:
            return CF_NOT_FOUND;
        case ENAMETOOLONG:
            return CF_TOO_LONG;
        case EACCES:
        case EPERM:
            return CF_NO_PERM;
        case ELOOP:
        case ENOTDIR:
            return CF_UNWALKABLE;
        default:
            D_ERROR("A system failure occurred when opening the config file", errnum);
            return CF_SYS_ERROR;
    }
}

static bool is_line_term(uint32_t code_point) {
    switch(code_point) {
        case 0x0A: 	 // LINE FEED
        case 0x0D:   // CARRIAGE RETURN
        case 0x85:   // NEXT LINE
        case 0x2028: // LINE SEPARATOR
        case 0x2029: // PARAGRAPH SEPARATOR
            return true;
            break;
    }
    return false;
}

static const uint16_t other_ws[] = {
    0xA0,   // NO-BREAK SPACE
    0x85,   // NEXT LINE
    0x1680, // OGHAM SPACE MARK
    0x2028, // LINE SEPARATOR
    0x2029, // PARAGRAPH SEPARATOR
    0x202F, // NARROW NO-BREAK SPACE
    0x205F, // MEDIUM MATHEMATICAL SPACE
    0x3000, // IDEOGRAPHIC SPACE
    0xFEFF, // ZERO WIDTH NO-BREAK SPACE
};

static bool is_white_space(uint32_t code_point) {
    // Checks for:
    // 0x20: Space
    // 0x09: Tab
    // 0x0A: LINE FEED
    // 0x0B: Line Tabulation
    // 0x0C: FORM FEED
    // 0x0D: CARRIAGE RETURN
    if((code_point == 0x20) || (code_point >= 0x09 && code_point <= 0x0D)) {
        return true;
    }
    // Covers Characters EN QUAD to HAIR SPACE
    if(code_point >= 0x2000 && code_point <= 0x200A) {
        return true;
    }
    for(uint8_t i = 0; i < sizeof(other_ws)/sizeof(uint16_t); i++) {
        uint32_t cur = other_ws[i];
        if(code_point == cur) {
            return true;
        }
    }
    return false;
}

static uint8_t* eat_white_space(uint8_t* str) {
    while(*str != '\0') {
        const uint8_t* nxt = str;
        uint32_t cpt = utf8_next_padded_unchecked(&nxt);
        if(!is_white_space(cpt)) {
            break;
        }
        str = (uint8_t*)nxt;
    }
    return str;
}

static bool is_comment(uint8_t* str) {
    str = eat_white_space(str);
    return *str == '#';
}

static uint8_t* next_line_priv(ConFile *cf) {
    uint8_t* dat = cf->data;
    size_t   cur = cf->cursor;
    size_t   sz  = cf->size;

    if(cur >= sz) {
        return nullptr;
    }

    uint8_t* ln  = dat + cur;
    size_t   len = 0;
    uint8_t* pos = ln;
    uint8_t* end = dat + sz;

    while(pos < end) {
        const uint8_t* nxt = pos;
        uint32_t cpt = utf8_next_padded_unchecked(&nxt);
        if(is_line_term(cpt)) {
            nxt += (cpt == 0x0D && (*nxt) == 0x0A);
            len = nxt - pos;
            break;
        }
        pos = (uint8_t*)nxt;
    }

    if(len == 0) {
        // I over allocate in new by 5 and set it to '\0'.
        // So I just need to update only the cursor.
        cf->cursor = sz;
        return ln;
    }

    // Zero out the entire new line.
    for (size_t i = 0; i < len; i++) {
        pos[i] = '\0';
    }
    cf->cursor = (pos - dat) + len;
    return ln;
}

static uint8_t parse_quote_str(uint8_t* str, uint8_t** after) {
    if(*str != '"') {
        return Q_UNOPENED;
    }

    uint8_t* w = str;     // write
    uint8_t* r = str + 1; // read, add one to skip the opening `"`
    
    for(;;) {
        uint8_t c = *r;
        if(c == '\0') {
            return Q_UNCLOSED;
        }
        // Closed/terminated
        if(c == '"') {
            *w = '\0';
            *after = r + 1;
            return Q_SUCCESS;
        }
        // Escape sequence
        if(c == '\\') {
            r += 1;
            switch(*r) {
                case '"':
                    c = '"';
                    break;
                case '\\':
                    c = '\\';
                    break;
                case 'n':
                    c = '\n';
                    break;
                case '\0':
                    return Q_UNCLOSED;
                default:
                    return Q_BAD_ESC;
            }
        }

        *w = c;
        r += 1;
        w += 1;
    }
}
