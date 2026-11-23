#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdlib.h>

#include "flex_arr.h"
#include "math_chk.h"

// +---------+-----------+-----------+---------+-----+-----+----------------------+
// | padding | alloc_off | max_align | el_size | cap | len | DATA[0]..DATA[CAP-1] |
// +---------+-----------+-----------+---------+-----+-----+----------------------+

const size_t HDR_SZ = sizeof(size_t) * 5;

static uint8_t* align_up(uint8_t* ptr, size_t alignment) {
    uintptr_t inc  = alignment - 1;
    uintptr_t msk  = ~inc;
    uintptr_t base = (uintptr_t)ptr;
    uintptr_t new  = (base + inc) & msk;
    return (uint8_t*)new;
}

static bool alloc_size(size_t el_sz, size_t max_align, size_t el_cnt, size_t* bytes) {
    size_t padding = max_align - 1;
    size_t amt;
    // Calculate array length in bytes
    if(usz_checked_mul(el_sz, el_cnt, &amt)) {
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

void* flex_arr_new(size_t el_size, size_t el_align, size_t capacity) {
    // Make sure it's a power of 2
    if(el_align == 0 || (el_align & (el_align - 1)) != 0) {
        return NULL;
    }

    size_t max_align = el_align > alignof(size_t) ? el_align : alignof(size_t);
    size_t bytes;
    if(alloc_size(el_size, max_align, capacity, &bytes)) {
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
    hdr[-2] = capacity;
    hdr[-3] = el_size;
    hdr[-4] = max_align;
    hdr[-5] = off;

    return start;
}

void flex_arr_free(void* arr) {
    if(arr == NULL) {
        return;
    }
    size_t off = ((size_t*)arr)[-5];
    free(((uint8_t*)arr) - off);
}
