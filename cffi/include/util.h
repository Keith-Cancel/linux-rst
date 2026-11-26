#ifndef MY_UTIL_FNS_H
#define MY_UTIL_FNS_H

#include <stddef.h>

// Mainly because I am annoyed that a memcpy with variable len does not want inline.
// Also unlike memcpy the return value is: dst + len
// Mainly want a small function.
#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
static __attribute__((always_inline)) inline void* imemcpy(void* dst, const void* src, size_t len) {
    // clang-format off
    __asm__ __volatile__(
        "rep movsb"
        : "+D"(dst), "+S"(src), "+c"(len)
        :
        : "memory"
    );
    // clang-format on
    return dst;
}
#else
static inline void* imemcpy(void* dst, const void* src, size_t len) {
    unsigned char*       d = dst;
    const unsigned char* s = src;
    // I want small not massive function, clang otherwise seems to explode
    #ifdef __clang__
        #pragma clang loop vectorize(disable) interleave(disable) unroll(disable)
    #endif
    // Backwards copy seems to reduce instruction count by 1 on gcc
    for(size_t i = len; i > 0; i--) {
        d[i - 1] = s[i - 1];
    }
    return d + len;
}
#endif

#endif
