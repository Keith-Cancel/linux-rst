#ifndef FLEXIBLE_ARRAY_TYPE_H
#define FLEXIBLE_ARRAY_TYPE_H


void flex_arr_free(void* arr);

/*
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// stdbool.h requires C99
#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
    
#endif


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




static void* flex_arr_new2(size_t el_size, size_t align, size_t capacity) {
    size_t max_align = align > alignof(size_t) ? align : alignof(size_t);

    size_t bytes = 0;
    if(flex_arr_alloc_sz(el_size, max_align, capacity, &bytes)) {
        return NULL;
    }

    size_t* data = malloc(bytes);
    if(data == NULL) {
        return NULL;
    }
    size_t* start = data + 4;
    start         = flex_arr_align_up(start, max_align);
    *(start - 1)  = 0;
    *(start - 2)  = capacity;
    *(start - 3)  = el_size;
    *(start - 4)  = max_align;
    return start;
}*/

#endif
