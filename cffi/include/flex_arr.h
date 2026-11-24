#ifndef FLEXIBLE_ARRAY_TYPE_H
#define FLEXIBLE_ARRAY_TYPE_H

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>

#define flex_arr_new(TYPE, capacity) (TYPE*)(flex_arr_new_ext((capacity), sizeof(TYPE), alignof(TYPE)))

void* flex_arr_new_ext(size_t capacity, size_t element_size, size_t alignment);
void* flex_arr_expand(void* arr, size_t element_size, size_t align);
void  flex_arr_free(void* flex_arr, size_t align);

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

// If align <= USZ_SZ_ALIGN:
// - alignment:   USZ_SZ_ALIGN
// - extra space: 2 * USZ_SZ_ALIGN;
//
// if align > USZ_SZ_ALIGN:
// - alignment:   align
// - extra space: align
// This works because the next alignment larger
// always be 2 * USZ_SZ_ALIGN so can store two
// USZ_SZ_ALIGN (e.g size_t)
static inline size_t flex_arr_max_align(size_t align) {
    return align > USZ_SZ_ALIGN ? align : USZ_SZ_ALIGN;
}

static inline size_t flex_arr_ext_space(size_t align) {
    return align > USZ_SZ_ALIGN ? align : 2 * USZ_SZ_ALIGN;
}


static inline size_t flex_arr_len(const void* flex_arr) {
    const char* ptr = flex_arr;

    size_t len;
    memcpy(&len, ptr - USZ_SZ_ALIGN, sizeof(size_t));
    return len;
}

static inline size_t flex_arr_capacity(const void* flex_arr) {
    const char* ptr = flex_arr;

    size_t cap;
    memcpy(&cap, ptr - (2 * USZ_SZ_ALIGN), sizeof(size_t));
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
        char* ptr      = ((char*)flex_arr) - USZ_SZ_ALIGN;         \
        memcpy(ptr, &len, sizeof(size_t));                         \
        return true;                                               \
    }

#define def_flex_arr_pop(TYPE, NAME)                             \
    static bool flex_arr_pop_##NAME(TYPE* flex_arr, TYPE* out) { \
        size_t len = flex_arr_len(flex_arr);                     \
        if(len < 1) {                                            \
            return false;                                        \
        }                                                        \
        len -= 1;                                                \
        if(out != NULL) {                                        \
            *out = flex_arr[len];                                \
        }                                                        \
        char* ptr = ((char*)flex_arr) - USZ_SZ_ALIGN;            \
        memcpy(ptr, &len, sizeof(size_t));                       \
        return true;                                             \
    }

#endif
