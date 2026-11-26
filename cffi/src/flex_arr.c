#include "flex_arr.h"
#include "math_chk.h"
#include "xmem.h"

// Flex Array memory layout
// Cap and len use USZ_SZ_ALIGN amount of space.
// +---------+-----+-----+----------------------+
// | padding | cap | len | DATA[0]..DATA[CAP-1] |
// +---------+-----+-----+----------------------+

static inline void* flex_arr_alloc(usz capacity, usz element_size, usz align, usz len) {
    // Make sure it's a power of 2
    if(align == 0 || (align & (align - 1)) != 0) {
        return NULL;
    }
    usz max_align = farr_max_align(align);
    usz extra_spc = farr_ext_space(align);
    usz bytes;
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
    memcpy(start - USZ_SZ_ALIGN, &len, sizeof(usz));
    memcpy(start - (2 * USZ_SZ_ALIGN), &capacity, sizeof(usz));
    return start;
}


void* farr_new_ext(usz capacity, usz element_size, usz align) {
    return flex_arr_alloc(capacity, element_size, align, 0);
}

void* farr_expand(void* arr, usz element_size, usz align) {
    if(arr == NULL) {
        return NULL;
    }
    usz len     = farr_len(arr);
    usz old_cap = farr_capacity(arr);
    old_cap     = old_cap < 8 ? 8 : old_cap;
    // Try to increase by 50%
    usz new_cap = old_cap >> 1;
    if(usz_checked_add(new_cap, old_cap, &new_cap)) {
        return NULL;
    }

    char* start = flex_arr_alloc(new_cap, element_size, align, len);
    if(start == NULL) {
        return NULL;
    }
    // Copy over the data.
    usz amt = len * element_size;
    memcpy(start, arr, amt);

    farr_free_ext(arr, align);
    return start;
}

void farr_free_ext(void* arr, usz align) {
    if(arr == NULL) {
        return;
    }
    usz   extra_spc = farr_ext_space(align);
    char* ptr       = (char*)arr;
    xmem_free(ptr - extra_spc);
}
