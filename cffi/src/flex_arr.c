#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdalign.h>
#include <string.h>

#include "flex_arr.h"
#include "math_chk.h"

// In memory layout
// +---------+-----------+-----------+---------+-----+-----+----------------------+
// | padding | alloc_off | max_align | el_size | cap | len | DATA[0]..DATA[CAP-1] |
// +---------+-----------+-----------+---------+-----+-----+----------------------+

typedef struct flex_hdr_s {
    size_t m_off;
    size_t align;
    size_t el_sz;
    size_t cap;
    size_t len;
} FlexHdr;

// Probably unesscary because all the types are the same in the struct, but does not hurt.
_Static_assert(sizeof(FlexHdr) == sizeof(size_t) * 5, "Unexpected struct padding");
_Static_assert(alignof(FlexHdr) == alignof(size_t), "Unexpected alignment");

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
    if(usz_checked_add(amt, sizeof(FlexHdr), &amt)) {
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
    uint8_t* start = align_up(mem + sizeof(FlexHdr), max_align);
    size_t   off   = start - mem;
    FlexHdr* hdr   = ((FlexHdr*)start) - 1;

    hdr->len   = 0;
    hdr->cap   = cap;
    hdr->el_sz = el_sz;
    hdr->align = max_align;
    hdr->m_off = off;

    return start;
}

void* flex_arr_new(size_t capacity, size_t el_size, size_t el_align) {
    // Make sure it's a power of 2
    if(el_align == 0 || (el_align & (el_align - 1)) != 0) {
        return NULL;
    }
    size_t max_align = el_align > alignof(FlexHdr) ? el_align : alignof(FlexHdr);
    return alloc_mem(capacity, el_size, max_align);
}

void* flex_arr_expand(void* arr) {
    if(arr == NULL) {
        return NULL;
    }
    FlexHdr* hdr     = ((FlexHdr*)arr) - 1;
    size_t   old_cap = hdr->cap;
    // There is a case of if old cap is 1 or 0 a right shift
    // by 1 will just be zero so just always make sure that
    // larger than those by choosing 16 here.
    old_cap = old_cap < 16 ? 16 : old_cap;
    // Try to increase by 50%
    size_t new_cap = old_cap >> 1;
    if(usz_checked_add(new_cap, old_cap, &new_cap)) {
        return NULL;
    }

    uint8_t* new_mem = alloc_mem(new_cap, hdr->el_sz, hdr->align);
    if(new_mem == NULL) {
        return NULL;
    }
    // Can't overflow we previously allocated el_sz * cap and len <= cap
    size_t len   = hdr->len;
    size_t bytes = len * hdr->el_sz;
    // Copy the old data over;
    memcpy(new_mem, arr, bytes);
    ((size_t*)new_mem)[-1] = len;

    flex_arr_free(hdr + 1);
    return new_mem;
}

void flex_arr_free(void* arr) {
    if(arr == NULL) {
        return;
    }
    size_t off = (((FlexHdr*)arr) - 1)->m_off;
    free(((uint8_t*)arr) - off);
}
