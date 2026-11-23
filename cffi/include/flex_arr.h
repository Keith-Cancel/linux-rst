#ifndef FLEXIBLE_ARRAY_TYPE_H
#define FLEXIBLE_ARRAY_TYPE_H

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>

#define flex_arr_new(TYPE, capacity) (TYPE*)(flex_arr_new_ext((capacity), sizeof(TYPE), alignof(TYPE)))

void* flex_arr_new_ext(size_t capacity, size_t el_size, size_t el_align);
void* flex_arr_expand(void* flex_arr);
void  flex_arr_free(void* flex_arr);

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

#endif
