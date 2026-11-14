#include "diag.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <stdatomic.h>

// Defined in strerror_r.c
int strerror_r_wrap(int errnum, char *buf, size_t buflen);

static const char LVL_STR[8][20] = {
    [DIAG_DEBUG]      = "[DEBUG] ",
    [DIAG_INFO]       = "[INFO ] ",
    [DIAG_WARN]       = "[WARN ] ",
    [DIAG_ERROR]      = "[ERROR] ",
    [DIAG_DEBUG + 4]  = "[\x1b[1;35mDEBUG\x1b[0m] ",
    [DIAG_INFO  + 4]  = "[\x1b[1;34mINFO\x1b[0m ] ",
    [DIAG_WARN  + 4]  = "[\x1b[1;93mWARN\x1b[0m ] ",
    [DIAG_ERROR + 4]  = "[\x1b[1;31mERROR\x1b[0m] "
};

// Global configuration
static _Atomic uint32_t _level = DIAG_WARN;
static _Atomic bool     _color = false;

static DiagLvl get_level(void) { 
    return atomic_load_explicit(&_level, memory_order_relaxed);
}

static bool get_color(void) {
    return atomic_load_explicit(&_color, memory_order_relaxed);
}

static void set_level(DiagLvl level) { 
    atomic_store_explicit(&_level, level, memory_order_relaxed);
}

static void set_color(bool use_color) { 
    atomic_store_explicit(&_color, use_color, memory_order_relaxed);
}

void diag_init(DiagLvl level, bool use_color) {
    set_level(level);
    set_color(use_color);
}

// Can write up to 19 bytes.
static size_t add_level(uint8_t* buff, DiagLvl lvl) {
    size_t color = get_color();
    size_t idx = lvl & 0x3;
    if(!color) {
        memcpy(buff, LVL_STR[idx], 8);
    } else {
        memcpy(buff, LVL_STR[idx + 4], 19);
    }
    return 8 + (color * 11);
}

// Can write up to 26 bytes.
static size_t add_file(uint8_t* buff, const char* file, uint32_t line) {
    const size_t WIDTH = 16;
    char    ln[11];
    bool    dot    = false;
    bool    color  = get_color();
    size_t  ln_len = snprintf(ln, 11, "%"PRIu32, line);
    size_t  len    = strlen(file);

    // Adjust file name if over visible length (name ':' line)
    if((len + ln_len + 1) > WIDTH) {
        size_t new_len = WIDTH - (ln_len + 1);
        file += (len - new_len);
        len = new_len;
        dot = true;
    }

    uint8_t* pos = buff;
    if(color) {
        memcpy(pos, &(uint8_t[4]) { 0x1b, '[', '2', 'm' }, 4);
        pos += 4;
    }

    // Copy file name and line number to buffer
    memcpy(pos, file, len);
    if(dot) {
        memset(pos, '.', 3);
    }

    pos[len] = ':';
    pos += len + 1;

    memcpy(pos, ln, ln_len);
    pos += ln_len;

    if(color) {
        memcpy(pos, &(uint8_t[4]) { 0x1b, '[', '0', 'm' }, 4);
        pos += 4;
    }
    memcpy(pos, ": ", 2);
    return (pos + 2) - buff;
}

// Will always return at most len - 1 buffer space used.
static size_t write_errno(uint8_t* buff, size_t len, int errnum) {
    // snprintf and strerror_r always write a null byte byte
    // if the buffer is larger enough to write even a byte.
    // we always exclude that so it will be len - 1 at most.
    // Enough to always ensure I can write a new line.
    if(len == 0) {
        return 0;
    }
    uint8_t* pos = buff;
    int      amt = snprintf((char*)pos, len, ": Errno(%d): ", errnum);
    if(amt < 0) {
        return 0;
    }
    if((size_t)amt >= len) {
        // Subtract 1 for the null byte.
        // Additionally, the saves a byte for the new line.
        return len - 1;
    }
    pos  += amt;
    len  -= amt;

    errno = 0;
    // Does not have a defined error return value only sets errno on failure.
    strerror_r_wrap(errnum, (char*)pos, len);
    if(errno == 0) {
        // find end of str.
        uint8_t* end = pos + len;
        while(pos < end && *pos != '\0') {
            pos += 1;
        }
    }

    return pos - buff;
}

#define BUFF_SZ 512
void diag_emit(DiagLvl level, const char* file, uint32_t line, const char* msg, int errnum) {
    uint8_t buff[BUFF_SZ];
    if(level < get_level()) {
        return;
    }

    uint8_t* pos = buff;
    pos += add_level(pos, level);
    pos += add_file(pos, file, line);

    // Add 1 so there is room for new line.
    size_t free = BUFF_SZ - ((pos + 1) - buff);
    for(size_t i = 0; i < free; i++) {
        char c = msg[i];
        if(c == '\0') {
            break;
        }
        *pos = c;
        pos += 1;
    }

    // If success was provided we want to print it.
    if(errnum >= 0) {
        // write_errno will only ever consume
        // free - 1 so we always have space for the new line.
        free = BUFF_SZ - (pos - buff);
        pos += write_errno(pos, free, errnum);
    }

    // Add new line.
    *pos = '\n';
    pos += 1;
    fwrite(buff, 1, pos - buff, stderr);
}