#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "conf.h"
#include "diag.h"


static uint8_t*   next_line_priv(ConFile *cf);
static ConFileErr cf_map_errno(int errnum);
static ConFileErr parse_command(uint8_t* line);

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
    cf->data   = malloc(cf->size + 1);
    if(cf->data == nullptr) {
        free(cf);
        close(fd);
        return CF_NO_MEM;
    }
    cf->data[cf->size] = '\0';

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
    return CF_SUCCESS;
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

static uint8_t* next_line_priv(ConFile *cf) {
    uint8_t* dat = cf->data;
    size_t   cur = cf->cursor;
    size_t   sz  = cf->size;

    if(cur >= sz) {
        return nullptr;
    }

    size_t rem  = sz - cur;
    uint8_t* ln = dat + cur;
    uint8_t* nl = memchr(ln, '\n', rem);
    
    if(nl != nullptr) {
        *nl = '\0';
        // Check for carriage return.
        if(nl > ln && *(nl - 1) == '\r') {
            *(nl - 1) = '\0'; // Also null out.
        }
        cf->cursor = (nl - dat) + 1;
    } else {
        // I over allocate in new by 1 and set to '\0'
        // so don't need to anything here but update
        // the curose.
        cf->cursor = sz;
    }
    return ln;
}