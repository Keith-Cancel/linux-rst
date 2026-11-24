#include "flex_arr.h"
#include "math_chk.h"
#include "xmem.h"

// Flex Array memory layout
// Cap and len use USZ_SZ_ALIGN amount of space.
// +---------+-----+-----+----------------------+
// | padding | cap | len | DATA[0]..DATA[CAP-1] |
// +---------+-----+-----+----------------------+

static inline void* flex_arr_alloc(size_t capacity, size_t element_size, size_t align, size_t len) {
    // Make sure it's a power of 2
    if(align == 0 || (align & (align - 1)) != 0) {
        return NULL;
    }
    size_t max_align = flex_arr_max_align(align);
    size_t extra_spc = flex_arr_ext_space(align);
    size_t bytes;
    if(usz_checked_mul(element_size, capacity, &bytes)) {
        return NULL;
    }
    if(usz_checked_add(bytes, extra_spc, &bytes)) {
        return NULL;
    }

    char* mem = xmem_aligned(max_align, bytes);
    if(mem == NULL) {
        return NULL;
    }
    char* start = mem + extra_spc;
    // Write capacity and length.
    memcpy(start - USZ_SZ_ALIGN, &len, sizeof(size_t));
    memcpy(start - (2 * USZ_SZ_ALIGN), &capacity, sizeof(size_t));
    return start;
}


void* flex_arr_new_ext(size_t capacity, size_t element_size, size_t align) {
    return flex_arr_alloc(capacity, element_size, align, 0);
}

void* flex_arr_expand(void* arr, size_t element_size, size_t align) {
    if(arr == NULL) {
        return NULL;
    }
    size_t len     = flex_arr_len(arr);
    size_t old_cap = flex_arr_capacity(arr);
    old_cap        = old_cap < 8 ? 8 : old_cap;
    // Try to increase by 50%
    size_t new_cap = old_cap >> 1;
    if(usz_checked_add(new_cap, old_cap, &new_cap)) {
        return NULL;
    }

    char* start = flex_arr_alloc(new_cap, element_size, align, len);
    if(start == NULL) {
        return NULL;
    }
    // Copy over the data.
    size_t amt = len * element_size;
    memcpy(start, arr, amt);

    flex_arr_free(arr, align);
    return start;
}

void flex_arr_free(void* arr, size_t align) {
    if(arr == NULL) {
        return;
    }
    size_t extra_spc = flex_arr_ext_space(align);
    char*  ptr       = (char*)arr;
    xmem_free(ptr - extra_spc);
}
