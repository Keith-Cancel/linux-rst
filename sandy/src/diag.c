#include "diag.h"
#include <stdatomic.h>
#include <stdbool.h>

static const char LVL_STR [4][8] = {
    [DIAG_DEBUG] = "[DEBUG]",
    [DIAG_INFO]  = "[INFO ]",
    [DIAG_WARN]  = "[WARN ]",
    [DIAG_ERROR] = "[ERROR]"
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

void diag_emit(DiagLvl level, const char* file, uint32_t line, const char* msg) {
    if(level < get_level()) {
        return;
    }
    //[DEBUG]
    //[INFO ]
    //[WARN ]
    //[ERROR]
}