#ifndef MY_COMPAT_HEADER_H
#define MY_COMPAT_HEADER_H

#include <stddef.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
/* alignof is a keyword in C23 */
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #ifndef __alignof_is_defined
        /* Just use the macro from stdalign.h */
        #include <stdalign.h>
    #endif
#elif defined(_MSC_VER)
    #define alignof(TYPE) __alignof(TYPE)
#elif (defined(__GNUC__) && __GNUC__ >= 2) || defined(__clang__)
    #define alignof(TYPE) __extension__ __alignof__(TYPE)
#else
    /* This is kinda of a best effor attempt since in this case
    ** I don't have any compiler features to rely on.
    */
    /* clang-format off */
    #define alignof(TYPE) ((size_t)(((char*)&(((struct { char a; TYPE t; }*)0)->t)) - (char*)0))
/* clang-format on */
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
/* bool, true, and false are keywords in C23 */
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #ifndef __bool_true_false_are_defined
        #include <stdbool.h>
    #endif
#else
    #ifndef true
        #define true 1
    #endif
    #ifndef false
        #define false 0
    #endif

    /* clang-format off */
    #ifndef bool
        typedef unsigned char bool;
    #endif
    /* clang-format on */
#endif

/* clang-format off */
#ifndef MY_CHECKED_MATH_COMPAT
#define MY_CHECKED_MATH_COMPAT

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
    #elif (!defined(__clang__) && defined(__GNUC__) && (__GNUC__ >= 5)) || \
        (defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 8))) || \
        (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1900))
        #define HAS_BUILTIN_CHECKED_ADD 1
        #define HAS_BUILTIN_CHECKED_MUL 1
    #endif
#endif

#if (defined(__GNUC__) || defined(__clang__))
    #define COMPAT_FINLINE_FN static __inline__ __attribute__((__always_inline__))
#elif defined(_MSC_VER)
    #define COMPAT_FINLINE_FN static __forceinline
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define COMPAT_FINLINE_FN static inline
#else
    #define COMPAT_FINLINE_FN static
#endif

COMPAT_FINLINE_FN bool checked_add_sz(size_t a, size_t b, size_t* res) {
#if defined(HAS_C23_CHECKED_ARITH)
    return ckd_add(res, a, b);
#elif defined(HAS_BUILTIN_CHECKED_ADD)
    return __builtin_add_overflow(a, b, res);
#else
    size_t sum = a + b;
    *res       = sum;
    return sum < a;
#endif
}

COMPAT_FINLINE_FN bool checked_mul_sz(size_t a, size_t b, size_t* res) {
#if defined(HAS_C23_CHECKED_ARITH)
    return ckd_mul(res, a, b);
#elif defined(HAS_BUILTIN_CHECKED_MUL)
    return __builtin_mul_overflow(a, b, res);
#else
    const size_t SZ_MAX = (size_t)-1;
    *res = a * b;
    if(b != 0 && a > (SZ_MAX / b)) {
        return true;
    }
    return false;
#endif
}

#undef HAS_C23_CHECKED_ARITH
#undef HAS_BUILTIN_CHECKED_ADD
#undef HAS_BUILTIN_CHECKED_MUL
#undef COMPAT_FINLINE_FN

/* clang-format on */
#endif


#endif
