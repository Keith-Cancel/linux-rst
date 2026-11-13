#ifndef _VERBOSE_UTIL_H
#define _VERBOSE_UTIL_H

#include <stdint.h>

typedef struct verbose_s {
    uint32_t title_len: 8;
    uint32_t enabled:   1;
    uint32_t color:     1;
} Verbose;

static inline Verbose verbose_init(bool enabled, bool color) {
    return (Verbose) {
        .title_len = 16,
        .enabled   = enabled,
        .color     = color
    };
}

static inline bool verbose_enabled(Verbose v) {
    return v.enabled == 1;
}

static inline bool verbose_color(Verbose v) {
    return v.color == 1;
}

static inline uint8_t verbose_title_len(Verbose v) {
    return v.title_len;
}

static inline Verbose verbose_enable(Verbose v) {
    v.enabled = 1;
    return v;
}

static inline Verbose verbose_disable(Verbose v) {
    v.enabled = 0;
    return v;
}

static inline Verbose verbose_enable_color(Verbose v) {
    v.color = 1;
    return v;
}

static inline Verbose verbose_disable_color(Verbose v) {
    v.color = 0;
    return v;
}

static inline Verbose verbose_set_title_len(Verbose v, uint8_t len) {
    if(len > 32) {
        len = 32;
    }
    if(len < 2) {
        len = 2;
    }
    v.title_len = len;
    return v;
}

#endif