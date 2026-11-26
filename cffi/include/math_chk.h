#ifndef MATH_CHECKED_HEADER_H
#define MATH_CHECKED_HEADER_H

#include <stdbool.h>
#include "types.h"

#if (defined(__GNUC__) || defined(__clang__))
    #define COMPAT_FINLINE_FN static __inline__ __attribute__((__always_inline__))
#elif defined(_MSC_VER)
    #define COMPAT_FINLINE_FN static __forceinline
#else
    #define COMPAT_FINLINE_FN static inline
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
    #if defined(__has_include)
        #if __has_include(<stdckdint.h>)
            #include <stdckdint.h>
            #define HAS_C23_CHECKED_ARITH 1
        #endif
    #else
        #include <stdckdint.h>
        #define HAS_C23_CHECKED_ARITH 1
    #endif
#endif

#if !defined(HAS_C23_CHECKED_ARITH)
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_add_overflow)
            #define HAS_BUILTIN_CHECKED_ADD 1
        #endif
        #if __has_builtin(__builtin_mul_overflow)
            #define HAS_BUILTIN_CHECKED_MUL 1
        #endif
    #elif (!defined(__clang__) && defined(__GNUC__) && (__GNUC__ >= 5)) ||                                 \
        (defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 8))) || \
        (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1900))
        #define HAS_BUILTIN_CHECKED_ADD 1
        #define HAS_BUILTIN_CHECKED_MUL 1
    #endif
#endif

#ifdef HAS_C23_CHECKED_ARITH
COMPAT_FINLINE_FN bool usz_checked_add(usz a, usz b, usz* res) {
    return ckd_add(res, a, b);
}
COMPAT_FINLINE_FN bool u32_checked_add(u32 a, u32 b, u32* res) {
    return ckd_add(res, a, b);
}

COMPAT_FINLINE_FN bool usz_checked_mul(usz a, usz b, usz* res) {
    return ckd_mul(res, a, b);
}
COMPAT_FINLINE_FN bool u32_checked_mul(u32 a, u32 b, u32* res) {
    return ckd_mul(res, a, b);
}
#endif

#if HAS_BUILTIN_CHECKED_ADD
COMPAT_FINLINE_FN bool usz_checked_add(usz a, usz b, usz* res) {
    return __builtin_add_overflow(a, b, res);
}
COMPAT_FINLINE_FN bool u32_checked_add(u32 a, u32 b, u32* res) {
    return __builtin_add_overflow(a, b, res);
}
#elif !defined(HAS_C23_CHECKED_ARITH)
COMPAT_FINLINE_FN bool usz_checked_add(usz a, usz b, usz* res) {
    usz sum = a + b;
    *res    = sum;
    return sum < a;
}
COMPAT_FINLINE_FN bool u32_checked_add(u32 a, u32 b, u32* res) {
    usz sum = a + b;
    *res    = sum;
    return sum < a;
}
#endif


#ifdef HAS_BUILTIN_CHECKED_MUL
COMPAT_FINLINE_FN bool usz_checked_mul(usz a, usz b, usz* res) {
    return __builtin_mul_overflow(a, b, res);
}
COMPAT_FINLINE_FN bool u32_checked_mul(u32 a, u32 b, u32* res) {
    return __builtin_mul_overflow(a, b, res);
}
#elif !defined(HAS_C23_CHECKED_ARITH)
COMPAT_FINLINE_FN bool usz_checked_mul(usz a, usz b, usz* res) {
    const usz USZ_MAX = (usz)-1;
    *res              = a * b;
    return (b != 0 && a > (USZ_MAX / b));
}
COMPAT_FINLINE_FN bool u32_checked_mul(u32 a, u32 b, u32* res) {
    const u32 U32_MAX = (u32)-1;
    *res              = a * b;
    return (b != 0 && a > (U32_MAX / b));
}
#endif

#undef HAS_C23_CHECKED_ARITH
#undef HAS_BUILTIN_CHECKED_ADD
#undef HAS_BUILTIN_CHECKED_MUL
#undef COMPAT_FINLINE_FN

#endif
