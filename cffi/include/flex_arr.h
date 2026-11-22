#ifndef FLEXIBLE_ARRAY_TYPE_H
#define FLEXIBLE_ARRAY_TYPE_H

#include <stdlib.h>
#include <stdbool.h>

// stdbool.h requires C99
#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
    #error "This header requires C99 or later."
#endif

#if (__STDC_VERSION__ >= 202311L)
    #if defined(__has_include)
        #if __has_include(<stdckdint.h>)
            #include <stdckdint.h>
            #define C23_CHECKED_ARTIH__ 1
        #endif
    #else
        #include <stdckdint.h>
        #define C23_CHECKED_ARTIH__ 1
    #endif
#endif

#if !defined(C23_CHECKED_ARTIH__)
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_add_overflow)
            #define BUILTIN_CHECKED_ADD__ 1
        #endif
        #if __has_builtin(__builtin_mul_overflow)
            #define BUILTIN_CHECKED_MUL__ 1
        #endif
    #elif !defined(__clang__) && (defined(__GNUC__) && (__GNUC__ >= 5)) || \
        (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1900))
        #define BUILTIN_CHECKED_ADD__ 1
        #define BUILTIN_CHECKED_MUL__ 1
    #endif
#endif

static inline bool flex_arr_add(size_t a, size_t b, size_t* res) {
#if defined(C23_CHECKED_ARTIH__)
    return ckd_add(res, a, b);
#elif defined(BUILTIN_CHECKED_ADD__)
    return __builtin_add_overflow(a, b, res);
#else
    size_t sum = a + b;
    *res       = sum;
    return sum < a;
#endif
}

static inline bool flex_arr_mul(size_t a, size_t b, size_t* res) {
#if defined(C23_CHECKED_ARTIH__)
    return ckd_mul(res, a, b);
#elif defined(BUILTIN_CHECKED_MUL__)
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

#undef C23_CHECKED_ARTIH__
#undef BUILTIN_CHECKED_ADD__
#undef BUILTIN_CHECKED_MUL__


#define DEFINE_FLEX_ARR(TYPE, NAME)                              \
    typedef struct flex_arr_##NAME##_s {                         \
        TYPE*  data;                                             \
        size_t size;                                             \
        size_t capacity;                                         \
    } NAME;                                                      \
                                                                 \
    static inline bool NAME##_init(NAME* arr, size_t capacity) { \
        size_t bytes = 0;                                        \
        if(flex_arr_mul(sizeof(TYPE), capacity, &bytes)) {       \
            return false;                                        \
        }                                                        \
        TYPE* data = malloc(bytes);                              \
        if(data == NULL) {                                       \
            return false;                                        \
        }                                                        \
        arr->size     = 0;                                       \
        arr->capacity = capacity;                                \
        arr->data     = data;                                    \
        return true;                                             \
    }                                                            \
                                                                 \
    static inline void NAME##_free(NAME* arr) {                  \
        if(arr->data == NULL) {                                  \
            return;                                              \
        }                                                        \
        free(arr->data);                                         \
        arr->data     = NULL;                                    \
        arr->size     = 0;                                       \
        arr->capacity = 0;                                       \
    }


#endif
