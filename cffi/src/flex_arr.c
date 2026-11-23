#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "flex_arr.h"
#include "math_chk.h"
/*
// In memory layout
// +---------+-----------+-----------+---------+-----+-----+----------------------+
// | padding | alloc_off | max_align | el_size | cap | len | DATA[0]..DATA[CAP-1] |
// +---------+-----------+-----------+---------+-----+-----+----------------------+

static const size_t HDR_SZ = 5 * sizeof(size_t);

#define MEM_OFF   0
#define MAX_ALIGN 1
#define ELE_SIZE  2
#define ARR_CAP   3
#define ARR_LEN   4

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

static uint8_t* alloc_mem(size_t hdr[5]) {
    size_t bytes;
    if(alloc_size(hdr[ARR_CAP], hdr[ELE_SIZE], hdr[MAX_ALIGN], &bytes)) {
        return NULL;
    }
    uint8_t* mem = malloc(bytes);
    if(mem == NULL) {
        return NULL;
    }
    uint8_t* arr = align_up(mem + HDR_SZ, hdr[MAX_ALIGN]);
    hdr[MEM_OFF] = arr - mem;
    memcpy(arr - HDR_SZ, hdr, HDR_SZ);
    return arr;
}


void* flex_arr_new_ext(size_t capacity, size_t size, size_t align) {
    // Make sure it's a power of 2
    if(align == 0 || (align & (align - 1)) != 0) {
        return NULL;
    }
    size_t hdr[5] = {
        [MAX_ALIGN] = align > alignof(size_t) ? align : alignof(size_t),
        [ELE_SIZE]  = size,
        [ARR_CAP]   = capacity,
        [ARR_LEN]   = 0,
    };
    return alloc_mem(hdr);
}

static uint8_t* alloc_mem2(const size_t* hdr, size_t* off) {
    size_t bytes;
    if(alloc_size(hdr[ARR_CAP], hdr[ELE_SIZE], hdr[MAX_ALIGN], &bytes)) {
        return NULL;
    }
    uint8_t* mem = malloc(bytes);
    if(mem == NULL) {
        return NULL;
    }
    uint8_t* arr = align_up(mem + HDR_SZ, hdr[MAX_ALIGN]);

    *off = arr - mem;
    return mem;
}

void* flex_arr_expand(void* arr) {
    if(arr == NULL) {
        return NULL;
    }
    size_t hdr[5];
    memcpy(hdr, ((uint8_t*)arr) - HDR_SZ, HDR_SZ);
    size_t old_cap = hdr[ARR_CAP];
    // There is a case of if old cap is 1 or 0 a right shift
    // by 1 will just be zero so just always make sure that
    // larger than those by choosing 16 here.
    old_cap = old_cap < 16 ? 16 : old_cap;
    // Try to increase by 50%
    size_t new_cap = old_cap >> 1;
    if(usz_checked_add(new_cap, old_cap, hdr + ARR_CAP)) {
        return NULL;
    }
    // Can't overflow we previously allocated el_sz * cap and len <= cap
    size_t   bytes   = hdr[ARR_LEN] * hdr[ELE_SIZE];
    uint8_t* new_arr = alloc_mem(hdr);
    if(new_arr == NULL) {
        return NULL;
    }
    // Copy the old data over;
    memcpy(new_arr, arr, bytes);
    flex_arr_free(arr);
    return new_arr;
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
const size_t USZ_SZ_ALIGN = sizeof(size_t) <= 1   ? 1
                            : sizeof(size_t) <= 2 ? 2
                            : sizeof(size_t) <= 4 ? 4
                            : sizeof(size_t) <= 8 ? 8
                                                  : 16;
_Static_assert(sizeof(size_t) <= 16, "Unexpected sizeof(size_t) larger 16 maybe a 128 bit machine...");

void* flex_arr_new_ext(size_t capacity, size_t element_size, size_t align) {
    // Make sure it's a power of 2
    if(align == 0 || (align & (align - 1)) != 0) {
        return NULL;
    }
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
    size_t max_align = USZ_SZ_ALIGN;
    size_t extra_spc = 2 * USZ_SZ_ALIGN;
    if(align > USZ_SZ_ALIGN) {
        max_align = align;
        extra_spc = align;
    }

    size_t bytes;
    if(usz_checked_mul(element_size, capacity, &bytes)) {
        return NULL;
    }
    if(usz_checked_add(bytes, extra_spc, &bytes)) {
        return NULL;
    }

    uint8_t* mem = aligned_alloc(max_align, bytes);
    if(mem == NULL) {
        return NULL;
    }
    uint8_t* start = mem + extra_spc;

    size_t len = 0;
    memcpy(start - USZ_SZ_ALIGN, &len, sizeof(size_t));
    memcpy(start - (2 * USZ_SZ_ALIGN), &capacity, sizeof(size_t));
    return start;
}

void flex_arr_free(void* arr, size_t align) {
    if(arr == NULL) {
        return;
    }
    size_t extra_spc = 2 * USZ_SZ_ALIGN;
    if(align > USZ_SZ_ALIGN) {
        extra_spc = align;
    }
    uint8_t* ptr = (uint8_t*)arr;
    free(ptr - extra_spc);
}
