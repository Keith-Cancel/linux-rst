#ifndef MY_TYPES_AND_TYPE_DEFS_HEADER_H
#define MY_TYPES_AND_TYPE_DEFS_HEADER_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef size_t  usz;
typedef ssize_t isz;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

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
    unsigned char char_s[4096];
    signed char   char_u[4096];

    short short_s[4096 / sizeof(short)];
    int   int_s[4096 / sizeof(int)];
    long  long_s[4096 / sizeof(long)];

    ushort short_u[4096 / sizeof(ushort)];
    uint   int_u[4096 / sizeof(uint)];
    ulong  long_u[4096 / sizeof(ulong)];

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    max_align_t align[4096 / sizeof(max_align_t)];
#endif
} Buff4k_t;


#endif
