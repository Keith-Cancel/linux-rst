#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "conf.h"
#include "diag.h"

static void print_err(const char* path, int errnum);

struct con_file_s {
    uint8_t* data;
    size_t   cursor;
    size_t   size;
};

ConFile* con_file_new(const char* path, Verbose verbose) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    int errnum = errno;

    verbose_line(verbose, "open", path, fd < 0);
    if(fd < 0) {
        print_err(path, errnum);
        return nullptr;
    }


    struct stat st = { 0 };
    int ret = fstat(fd, &st);
    errnum = errno;

    verbose_line(verbose, "fstat", path, ret < 0);
    if(ret < 0) {
        close(fd);
        D_ERROR("fstat() for the config file failed", errnum);
        return nullptr;
    }

    if(!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: Config file path is not a file: '%s'\n", path);
        close(fd);
        return nullptr;
    }

    ConFile *cf = malloc(sizeof(ConFile));
    if (cf == nullptr) {
        fprintf(stderr, "Error: Failed too allocate memory for config file: '%s'\n", path);
        close(fd);
        return nullptr;
    }

    cf->size   = (size_t)st.st_size;
    cf->cursor = 0;
    cf->data   = malloc(cf->size + 1);
    if(cf->data == nullptr) {
        fprintf(stderr, "Error: Failed too allocate memory for config file: '%s'\n", path);
        free(cf);
        close(fd);
        return nullptr;
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
    errnum = errno;
    close(fd);
    verbose_line(verbose,"read", path, rem > 0);

    if(rem > 0 && eof) {
        con_file_free(cf);
        fprintf(stderr, "Error: Config file unexpected EOF: '%s'\n", path);
        return nullptr;
    }
    if(rem > 0) {
        con_file_free(cf);
        D_ERROR("Failed reading config file", errnum);
        return nullptr;
    }
    return cf;
}

// Returns NULL when there are no more lines.
const uint8_t *con_file_next_line(ConFile *cf) {
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

void con_file_free(ConFile* file) {
    if(file == nullptr) {
        return;
    }
    if(file->data != nullptr) {
        free(file->data);
    }
    free(file);
}

static void print_err(const char* path, int errnum) {
    switch(errnum) {
        case ENOENT:
            fprintf(stderr, "Error: Config file not found: '%s'\n", path);
            break;
        case ENAMETOOLONG:
            fprintf(stderr, "Error: Config file path is too long: '%s'\n", path);
            break;
        case EACCES:
        case EPERM:
            fprintf(stderr, "Error: Permission denied for config file: '%s'\n", path);
            break;
        case ELOOP:
        case ENOTDIR:
            fprintf(stderr, "Error: A path component of the config file is not walkable: '%s'\n", path);
            break;
        default:
            D_ERROR("A system failure occurred when opening the config file", errnum);
            break;
    }
}