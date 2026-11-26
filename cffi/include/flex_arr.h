#ifndef FLEXIBLE_ARRAY_TYPE_H
#define FLEXIBLE_ARRAY_TYPE_H

#include <string.h>
#include <stdbool.h>
#include "types.h"

#define farr_new(TYPE, capacity) (TYPE*)(farr_new_ext((capacity), sizeof(TYPE), _Alignof(TYPE)))
#define farr_free(TYPE, array)   farr_free_ext((array), _Alignof(TYPE))

void* farr_new_ext(usz capacity, usz element_size, usz alignment);
// Similar to realloc with behavior void* arr and return value.
void* farr_expand(void* arr, usz element_size, usz align);
void  farr_free_ext(void* flex_arr, usz align);

// Basically I want a value that be an alignment and size.
// - Size: >= sizeof(usz), so it has enough space to store a usz.
// - Align: >= alignof(usz) and is a power of two.
//
// This produces a power of two value >= sizeof(usz).
// That is also a valid power of two alignment, because
// it will always a stricter alignment than alignof(usz).
// This is because sizeof(usz) is always a multiple
// of alignof(usz) so the next power of two >= sizeof(usz)
// will always >= alignof(usz).
//
// e.g. think about a hypothetical system with:
// - sizeof(usz) = 6
// - alignof(usz) = 2
// we would need 8 to be that value.
// clang-format off
static const usz USZ_SZ_ALIGN = sizeof(usz) <= 1 ? 1 :
                                sizeof(usz) <= 2 ? 2 :
                                sizeof(usz) <= 4 ? 4 :
                                sizeof(usz) <= 8 ? 8 : 16;
// clang-format on
_Static_assert(sizeof(usz) <= 16, "Unexpected sizeof(usz) larger 16 more than a 128 bit machine...");

// If align <= USZ_SZ_ALIGN:
// - alignment:   USZ_SZ_ALIGN
// - extra space: 2 * USZ_SZ_ALIGN;
//
// if align > USZ_SZ_ALIGN:
// - alignment:   align
// - extra space: align
// This works because the next alignment larger
// always be 2 * USZ_SZ_ALIGN so can store two
// USZ_SZ_ALIGN (e.g usz)
static inline usz farr_max_align(usz align) {
    return align > USZ_SZ_ALIGN ? align : USZ_SZ_ALIGN;
}

static inline usz farr_ext_space(usz align) {
    return align > USZ_SZ_ALIGN ? align : 2 * USZ_SZ_ALIGN;
}

static inline usz farr_len(const void* flex_arr) {
    const char* ptr = flex_arr;

    usz len;
    memcpy(&len, ptr - USZ_SZ_ALIGN, sizeof(usz));
    return len;
}

static inline usz farr_capacity(const void* flex_arr) {
    const char* ptr = flex_arr;

    usz cap;
    memcpy(&cap, ptr - (2 * USZ_SZ_ALIGN), sizeof(usz));
    return cap;
}

#define FARR_DEF_TRY_PUSH(TYPE, NAME)                              \
    static bool farr_try_push_##NAME(TYPE* flex_arr, TYPE value) { \
        usz len = farr_len(flex_arr);                              \
        if(len >= farr_capacity(flex_arr)) {                       \
            return false;                                          \
        }                                                          \
        flex_arr[len]  = value;                                    \
        len           += 1;                                        \
        char* ptr      = ((char*)flex_arr) - USZ_SZ_ALIGN;         \
        memcpy(ptr, &len, sizeof(usz));                            \
        return true;                                               \
    }

#define FARR_DEF_POP(TYPE, NAME)                             \
    static bool farr_pop_##NAME(TYPE* flex_arr, TYPE* out) { \
        usz len = farr_len(flex_arr);                        \
        if(len < 1) {                                        \
            return false;                                    \
        }                                                    \
        len -= 1;                                            \
        if(out != NULL) {                                    \
            *out = flex_arr[len];                            \
        }                                                    \
        char* ptr = ((char*)flex_arr) - USZ_SZ_ALIGN;        \
        memcpy(ptr, &len, sizeof(usz));                      \
        return true;                                         \
    }

// Similar to realloc with behavior void* arr and return value.
#define FARR_DEF_PUSH(TYPE, NAME)                                           \
    static inline TYPE* farr_push_##NAME(TYPE* flex_arr, TYPE value) {      \
        usz len = farr_len(flex_arr);                                       \
        if(len >= farr_capacity(flex_arr)) {                                \
            flex_arr = farr_expand(flex_arr, sizeof(TYPE), _Alignof(TYPE)); \
            if(flex_arr == NULL) {                                          \
                return NULL;                                                \
            }                                                               \
        }                                                                   \
        flex_arr[len]  = value;                                             \
        len           += 1;                                                 \
        char* ptr      = ((char*)flex_arr) - USZ_SZ_ALIGN;                  \
        memcpy(ptr, &len, sizeof(usz));                                     \
        return flex_arr;                                                    \
    }

#define FARR_DEF_LAST(TYPE, NAME)                   \
    static TYPE* farr_last_##NAME(TYPE* flex_arr) { \
        usz len = farr_len(flex_arr);               \
        if(len < 1) {                               \
            return NULL;                            \
        }                                           \
        return flex_arr + (len - 1);                \
    }


#endif
