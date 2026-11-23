#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdalign.h>
#include <string.h>

#include "flex_arr.h"
#include "math_chk.h"

// +---------+-----------+-----------+---------+-----+-----+----------------------+
// | padding | alloc_off | max_align | el_size | cap | len | DATA[0]..DATA[CAP-1] |
// +---------+-----------+-----------+---------+-----+-----+----------------------+

const size_t HDR_SZ = sizeof(size_t) * 5;

static inline uint8_t* align_up(uint8_t* ptr, size_t alignment) {
    uintptr_t inc  = alignment - 1;
    uintptr_t msk  = ~inc;
    uintptr_t base = (uintptr_t)ptr;
    uintptr_t new  = (base + inc) & msk;
    return (uint8_t*)new;
}

static bool alloc_size(size_t cap, size_t el_sz, size_t max_align, size_t* bytes) {
    size_t padding = max_align - 1;
    size_t amt;
    // Calculate array length in bytes
    if(usz_checked_mul(el_sz, cap, &amt)) {
        return true;
    }
    // Add the bytes needed for the five size_t fields.
    if(usz_checked_add(amt, HDR_SZ, &amt)) {
        return true;
    }
    // Add the padding that may be needed to align everything
    if(usz_checked_add(amt, padding, &amt)) {
        return true;
    }
    *bytes = amt;
    return false;
}

static uint8_t* alloc_mem(size_t cap, size_t el_sz, size_t max_align) {
    size_t bytes;
    if(alloc_size(cap, el_sz, max_align, &bytes)) {
        return NULL;
    }
    uint8_t* mem = malloc(bytes);
    if(mem == NULL) {
        return NULL;
    }
    uint8_t* start = align_up(mem + HDR_SZ, max_align);
    size_t   off   = start - mem;
    size_t*  hdr   = (size_t*)start;

    hdr[-1] = 0;
    hdr[-2] = cap;
    hdr[-3] = el_sz;
    hdr[-4] = max_align;
    hdr[-5] = off;

    return start;
}

void* flex_arr_new(size_t capacity, size_t el_size, size_t el_align) {
    // Make sure it's a power of 2
    if(el_align == 0 || (el_align & (el_align - 1)) != 0) {
        return NULL;
    }
    size_t max_align = el_align > alignof(size_t) ? el_align : alignof(size_t);
    return alloc_mem(capacity, el_size, max_align);
}

void* flex_arr_expand(void* arr) {
    if(arr == NULL) {
        return NULL;
    }
    size_t* hdr     = arr;
    size_t  old_cap = hdr[-2];
    // There is a case of if old cap is 1 or 0 a right shift
    // by 1 will just be zero so just always make sure that
    // larger than those by choosing 16 here.
    if(old_cap < 16) {
        old_cap = 16;
    }

    size_t new_cap = old_cap >> 1;
    // Try to increase by 50%
    if(usz_checked_add(new_cap, old_cap, &new_cap)) {
        return NULL;
    }

    uint8_t* new_mem = alloc_mem(new_cap, hdr[-3], hdr[-4]);
    if(new_mem == NULL) {
        return NULL;
    }
    // Can't overflow we previously allocated el_sz * cap and len <= cap
    size_t len   = hdr[-1];
    size_t bytes = len * hdr[-3];
    // Copy the old data over;
    memcpy(new_mem, arr, bytes);
    ((size_t*)new_mem)[-1] = len;

    flex_arr_free(hdr);
    return new_mem;
}

void flex_arr_free(void* arr) {
    if(arr == NULL) {
        return;
    }
    size_t off = ((size_t*)arr)[-5];
    free(((uint8_t*)arr) - off);
}
