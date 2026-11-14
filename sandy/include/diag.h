#ifndef _DIAG_UTIL_H
#define _DIAG_UTIL_H

#include <stdint.h>

typedef enum : uint8_t { 
    DIAG_DEBUG = 0,
    DIAG_INFO,
    DIAG_WARN,
    DIAG_ERROR,
    DIAG_NONE,
} DiagLvl;

void diag_init(DiagLvl level, bool use_color);
void diag_emit(DiagLvl level, const char* file, uint32_t line, const char* msg, int err);

// The print either just the message or message + errno.
#define D_DEBUG(msg, ...) _DIAG_EMIT(DIAG_DEBUG,  (msg) __VA_OPT__(,) __VA_ARGS__)
#define D_INFO(msg, ...)  _DIAG_EMIT(DIAG_INFO,   (msg) __VA_OPT__(,) __VA_ARGS__)
#define D_WARN(msg, ...)  _DIAG_EMIT(DIAG_WARN,   (msg) __VA_OPT__(,) __VA_ARGS__)
#define D_ERROR(msg, ...)  _DIAG_EMIT(DIAG_ERROR, (msg) __VA_OPT__(,) __VA_ARGS__)

#define _DIAG_EMIT_ARGS(msg, errno, ...) (msg), (errno)
#define _DIAG_EMIT(lvl, ...) \
    diag_emit( \
        (lvl), \
        __FILE__, \
        __LINE__, \
        _DIAG_EMIT_ARGS(__VA_ARGS__, -1) \
    )


#endif