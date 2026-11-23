#ifndef MY_COMPAT_HEADER_H
#define MY_COMPAT_HEADER_H

#include <stddef.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
/* alignof is a keyword in C23 */
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #if !defined(__alignof_is_defined)
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
        #define false 1
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
#if !defined(HAS_C23_CHECKED_ARTIH)
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_add_overflow)
            #define HAS_BUILTIN_CHECKED_ADD 1
        #endif
        #if __has_builtin(__builtin_mul_overflow)
            #define HAS_BUILTIN_CHECKED_MUL 1
        #endif
    #elif !defined(__clang__) && (defined(__GNUC__) && (__GNUC__ >= 5)) || \
        (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1900))
        #define HAS_BUILTIN_CHECKED_ADD 1
        #define HAS_BUILTIN_CHECKED_MUL 1
    #endif
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define COMPAT_INLINE_FN static inline
#elif defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
    #define COMPAT_INLINE_FN static __inline
#else
    #define COMPAT_INLINE_FN static
#endif

COMPAT_INLINE_FN bool checked_add_sz(size_t a, size_t b, size_t* res) {
#if defined(HAS_C23_CHECKED_ARTIH)
    return ckd_add(res, a, b);
#elif defined(HAS_BUILTIN_CHECKED_ADD)
    return __builtin_add_overflow(a, b, res);
#else
    size_t sum = a + b;
    *res       = sum;
    return sum < a;
#endif
}

COMPAT_INLINE_FN bool checked_mul_sz(size_t a, size_t b, size_t* res) {
#if defined(HAS_C23_CHECKED_ARTIH)
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

#undef HAS_C23_CHECKED_ARTIH
#undef HAS_BUILTIN_CHECKED_ADD
#undef HAS_BUILTIN_CHECKED_MUL
#undef COMPAT_INLINE_FN

/* clang-format on */
#endif


#endif
