#ifndef _VERBOSE_UTIL_H
#define _VERBOSE_UTIL_H

#include <stdint.h>

typedef struct verbose_s {
    uint32_t title_w: 8;
    uint32_t enabled: 1;
    uint32_t color:   1;
} Verbose;

typedef enum : uint32_t {
    V_OK   = 0,
    V_FAIL = 1,
    V_NONE = 2
} VStatus;

void verbose_header(Verbose v, const char* title);
void verbose_line(Verbose v, const char* title, const char* value, VStatus stat);

static inline Verbose verbose_init(bool enabled, bool color) {
    return (Verbose) {
        .title_w = 10,
        .enabled = enabled,
        .color   = color
    };
}

static inline bool verbose_enabled(Verbose v) {
    return v.enabled == 1;
}

static inline bool verbose_color(Verbose v) {
    return v.color == 1;
}

static inline uint8_t verbose_title_width(Verbose v) {
    return v.title_w;
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

// Clamped to minimum 2, and max 32.
static inline Verbose verbose_set_title_width(Verbose v, uint8_t width) {
    if(width > 32) {
        width = 32;
    }
    if(width < 2) {
        width = 2;
    }
    v.title_w = width;
    return v;
}

#endif