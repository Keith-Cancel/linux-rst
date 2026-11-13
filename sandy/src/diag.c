#include "diag.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdatomic.h>

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
    //memset(pos, ' ', WIDTH + 10);
    if(color) {
        memcpy(pos, &(uint8_t[4]) { 0x1b, '[', '1', 'm' }, 4);
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

#define BUFF_SZ 512
void diag_emit(DiagLvl level, const char* file, uint32_t line, const char* msg, bool use_errno) {
    uint8_t buff[BUFF_SZ];
    if(level < get_level()) {
        return;
    }

    uint8_t* pos = buff;
    pos += add_level(pos, level);
    pos += add_file(pos, file, line);

    *pos = 'a';
    *(pos + 1) = '\n';
    size_t amt = (pos + 2) - buff;
    fwrite(buff, 1, amt, stdout);
}
// src/diag.c:5:19