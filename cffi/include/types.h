#ifndef MY_TYPES_AND_TYPE_DEFS_HEADER_H
#define MY_TYPES_AND_TYPE_DEFS_HEADER_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

// Check standard integer types for one that match usize max
// clang-format off
#if defined(SIZE_MAX) && (SIZE_MAX == USHRT_MAX)
    typedef short isz;
    #define PRIdISZ "hd"
    #define PRIiISZ "hi"
    #define SCNdISZ "hd"
    #define SCNiISZ "hi"
#elif defined(SIZE_MAX) && (SIZE_MAX == UINT_MAX)
    typedef int isz;
    #define PRIdISZ "d"
    #define PRIiISZ "i"
    #define SCNdISZ "d"
    #define SCNiISZ "i"
#elif defined(SIZE_MAX) && (SIZE_MAX == ULONG_MAX)
    typedef long isz;
    #define PRIdISZ "ld"
    #define PRIiISZ "li"
    #define SCNdISZ "ld"
    #define SCNiISZ "li"
#elif defined(SIZE_MAX) && defined(ULLONG_MAX) && (SIZE_MAX == ULLONG_MAX)
    typedef long long isz;
    #define PRIdISZ "lld"
    #define PRIiISZ "lli"
    #define SCNdISZ "lld"
    #define SCNiISZ "lli"
#else
    // Guess fall back to ptrdiff.
    typedef ptrdiff_t isz;
    #define PRIdISZ "td"
    #define PRIiISZ "ti"
    #define SCNdISZ "td"
    #define SCNiISZ "ti"
#endif
// clang-format on

typedef size_t usz;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef intptr_t  iptr;
typedef uintptr_t uptr;

typedef unsigned short ushort;
typedef unsigned       uint;
typedef unsigned long  ulong;

typedef union buffer_4k_u {
    u8  u8[4096];
    u16 u16[4096 / sizeof(u16)];
    u32 u32[4096 / sizeof(u32)];
    u64 u64[4096 / sizeof(u64)];
    i8  i8[4096];
    i16 i16[4096 / sizeof(i16)];
    i32 i32[4096 / sizeof(i32)];
    i64 i64[4096 / sizeof(i64)];

    char          char_c[4096];
    unsigned char char_u[4096];
    signed char   char_s[4096];

    short short_s[4096 / sizeof(short)];
    int   int_s[4096 / sizeof(int)];
    long  long_s[4096 / sizeof(long)];

    ushort short_u[4096 / sizeof(ushort)];
    uint   int_u[4096 / sizeof(uint)];
    ulong  long_u[4096 / sizeof(ulong)];

    void* ptr[4096 / sizeof(void*)];

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    max_align_t align[4096 / sizeof(max_align_t)];
#endif
} Buff4k_t;

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert((isz)-1 < 0, "isz must be signed");
_Static_assert(sizeof(isz) == sizeof(size_t), "isz width must match size_t");
_Static_assert(_Alignof(isz) == _Alignof(size_t), "isz alignment must match size_t");

_Static_assert(sizeof(Buff4k_t) == 4096, "Buff4k_t is not 4096 bytes");
#endif

#endif
