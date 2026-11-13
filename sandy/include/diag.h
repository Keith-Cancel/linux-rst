#ifndef _DIAG_UTIL_H
#define _DIAG_UTIL_H

#include <stdint.h>

typedef enum : uint8_t { 
    DIAG_DEBUG = 0,
    DIAG_INFO,
    DIAG_WARN,
    DIAG_ERROR
} DiagLvl;

void diag_init(DiagLvl level, bool use_color);

typedef struct {
    const char* msg;
} DiagEvent;


#endif