#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "flex_arr.h"
#include "math_chk.h"

// In memory layout
// +---------+-----------+-----------+---------+-----+-----+----------------------+
// | padding | alloc_off | max_align | el_size | cap | len | DATA[0]..DATA[CAP-1] |
// +---------+-----------+-----------+---------+-----+-----+----------------------+

static const size_t HDR_SZ = 5 * sizeof(size_t);

static inline void set_len(uint8_t* arr, size_t len) {
    arr -= sizeof(size_t);
    memcpy(arr, &len, sizeof(size_t));
}

static inline void set_cap(uint8_t* arr, size_t cap) {
    arr -= 2 * sizeof(size_t);
    memcpy(arr, &cap, sizeof(size_t));
}

static inline void set_ele_sz(uint8_t* arr, size_t sz) {
    arr -= 3 * sizeof(size_t);
    memcpy(arr, &sz, sizeof(size_t));
}

static inline size_t get_ele_sz(uint8_t* arr) {
    arr -= 3 * sizeof(size_t);
    size_t sz;
    memcpy(&sz, arr, sizeof(size_t));
    return sz;
}

static inline void set_align(uint8_t* arr, size_t align) {
    arr -= 4 * sizeof(size_t);
    memcpy(arr, &align, sizeof(size_t));
}

static inline size_t get_align(uint8_t* arr) {
    arr -= 4 * sizeof(size_t);
    size_t align;
    memcpy(&align, arr, sizeof(size_t));
    return align;
}

static inline void set_mem_offset(uint8_t* arr, size_t off) {
    arr -= 5 * sizeof(size_t);
    memcpy(arr, &off, sizeof(size_t));
}

static inline size_t get_mem_offset(uint8_t* arr) {
    size_t off;
    arr -= 5 * sizeof(size_t);
    memcpy(&off, arr, sizeof(size_t));
    return off;
}

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

static uint8_t* alloc_mem(size_t cap, size_t ele_sz, size_t max_align) {
    size_t bytes;
    if(alloc_size(cap, ele_sz, max_align, &bytes)) {
        return NULL;
    }
    uint8_t* mem = malloc(bytes);
    if(mem == NULL) {
        return NULL;
    }
    uint8_t* arr = align_up(mem + HDR_SZ, max_align);
    size_t   off = arr - mem;

    set_len(arr, 0);
    set_cap(arr, cap);
    set_ele_sz(arr, ele_sz);
    set_align(arr, max_align);
    set_mem_offset(arr, off);
    return arr;
}

void* flex_arr_new_ext(size_t capacity, size_t el_size, size_t el_align) {
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
    size_t old_cap = flex_arr_capacity(arr);
    // There is a case of if old cap is 1 or 0 a right shift
    // by 1 will just be zero so just always make sure that
    // larger than those by choosing 16 here.
    old_cap = old_cap < 16 ? 16 : old_cap;
    // Try to increase by 50%
    size_t new_cap = old_cap >> 1;
    if(usz_checked_add(new_cap, old_cap, &new_cap)) {
        return NULL;
    }

    uint8_t* new_mem = alloc_mem(new_cap, get_ele_sz(arr), get_align(arr));
    if(new_mem == NULL) {
        return NULL;
    }
    // Can't overflow we previously allocated el_sz * cap and len <= cap
    size_t len   = flex_arr_len(arr);
    size_t bytes = len * get_ele_sz(arr);
    // Copy the old data over;
    memcpy(new_mem, arr, bytes);
    set_len(new_mem, len);

    flex_arr_free(arr);
    return new_mem;
}

void flex_arr_free(void* arr) {
    if(arr == NULL) {
        return;
    }
    size_t off = get_mem_offset(arr);
    free(((uint8_t*)arr) - off);
}
