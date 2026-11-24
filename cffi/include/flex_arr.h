#ifndef FLEXIBLE_ARRAY_TYPE_H
#define FLEXIBLE_ARRAY_TYPE_H

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>

#define flex_arr_new(TYPE, capacity) (TYPE*)(flex_arr_new_ext((capacity), sizeof(TYPE), alignof(TYPE)))

void* flex_arr_new_ext(size_t capacity, size_t element_size, size_t alignment);
void* flex_arr_expand(void* flex_arr);
void  flex_arr_free(void* flex_arr, size_t align);

/*
static inline size_t flex_arr_len(const void* flex_arr) {
    char*  ptr = ((char*)flex_arr) - sizeof(size_t);
    size_t len;
    memcpy(&len, ptr, sizeof(size_t));
    return len;
}

static inline size_t flex_arr_capacity(const void* flex_arr) {
    char*  ptr = ((char*)flex_arr) - (2 * sizeof(size_t));
    size_t cap;
    memcpy(&cap, ptr, sizeof(size_t));
    return cap;
}

#define def_flex_arr_push(TYPE, NAME)                              \
    static bool flex_arr_push_##NAME(TYPE* flex_arr, TYPE value) { \
        size_t len = flex_arr_len(flex_arr);                       \
        if(len >= flex_arr_capacity(flex_arr)) {                   \
            return false;                                          \
        }                                                          \
        flex_arr[len]  = value;                                    \
        len           += 1;                                        \
        char* ptr      = ((char*)flex_arr) - sizeof(size_t);       \
        memcpy(ptr, &len, sizeof(size_t));                         \
        return true;                                               \
    }

#define def_flex_arr_pop(TYPE, NAME)                             \
    static bool flex_arr_pop_##NAME(TYPE* flex_arr, TYPE* out) { \
        size_t len = flex_arr_len(flex_arr);                     \
        if(len < 1) {                                            \
            return false;                                        \
        }                                                        \
        len       -= 1;                                          \
        *out       = flex_arr[len];                              \
        char* ptr  = ((char*)flex_arr) - sizeof(size_t);         \
        memcpy(ptr, &len, sizeof(size_t));                       \
        return true;                                             \
    }
*/

// Basically I want a value that be an alignment and size.
// - Size: >= sizeof(size_t), so it has enough space to store a size_t.
// - Align: >= alignof(size_t) and is a power of two.
//
// This produces a power of two value >= sizeof(size_t).
// That is also a valid power of two alignment, because
// it will always a stricter alignment than alignof(size_t).
// This is because sizeof(size_t) is always a multiple
// of alignof(size_t) so the next power of two >= sizeof(size_t)
// will always >= alignof(size_t).
//
// e.g. think about a hypothetical system with:
// - sizeof(size_t) = 6
// - alignof(size_t) = 2
// we would need 8 to be that value.
// clang-format off
static const size_t USZ_SZ_ALIGN = sizeof(size_t) <= 1 ? 1 :
                                   sizeof(size_t) <= 2 ? 2 :
                                   sizeof(size_t) <= 4 ? 4 :
                                   sizeof(size_t) <= 8 ? 8 : 16;
// clang-format on
_Static_assert(sizeof(size_t) <= 16, "Unexpected sizeof(size_t) larger 16 more than a 128 bit machine...");

#endif
