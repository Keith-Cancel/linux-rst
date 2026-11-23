#ifndef FLEXIBLE_ARRAY_TYPE_H
#define FLEXIBLE_ARRAY_TYPE_H

#include <stddef.h>
#include <stdalign.h>

#define flex_arr_new(TYPE, capacity) flex_arr_new_ext((capacity), sizeof(TYPE), alignof(TYPE));

void* flex_arr_new_ext(size_t capacity, size_t el_size, size_t el_align);
void* flex_arr_expand(void* flex_arr);
void  flex_arr_free(void* flex_arr);

static inline size_t flex_arr_len(void* flex_arr) {
    return ((size_t*)flex_arr)[-1];
}

static inline size_t flex_arr_capacity(void* flex_arr) {
    return ((size_t*)flex_arr)[-2];
}

#endif
