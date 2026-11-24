#if (defined(__unix__) || defined(__APPLE__)) && (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L))
    #if !defined(_POSIX_C_SOURCE)
        #define _POSIX_C_SOURCE 200112L
    #endif
    #include <unistd.h>
#endif

#include <stdlib.h>
#include "xmem.h"

// clang-format off
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201710L)
    #define XMEM_DEFINED 1
    #define XMEM_ALLOC_INNER malloc
    #define XMEM_FREE_INNER  free
    // Don't need to make size a multiple of alignment:
    // https://www.open-std.org/jtc1/sc22/wg14/www/docs/summary.htm#dr_460
    #define XMEM_ALIGNED_INNER aligned_alloc
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    #define XMEM_DEFINED 1
    #define XMEM_ALLOC_INNER   malloc
    #define XMEM_FREE_INNER    free
    #define XMEM_ALIGNED_INNER xmem_aligned_inner
    static void* xmem_aligned_inner(size_t alignment, size_t size) {
        size_t msk = alignment - 1;
        // check if not a power of 2.
        if(alignment == 0 || (alignment & msk) != 0) {
            return NULL;
        }
        size_t sz = size + msk;
        // Overflowed.
        if(sz < size) {
            return NULL;
        }
        sz &= ~msk;
        return aligned_alloc(alignment, sz);
    }
#endif

#if !defined(XMEM_DEFINED) && defined(_POSIX_VERSION) && (_POSIX_VERSION >= 200112L)
    #define XMEM_DEFINED       1
    #define XMEM_ALLOC_INNER   malloc
    #define XMEM_FREE_INNER    free
    #define XMEM_ALIGNED_INNER xmem_aligned_inner
    static void* xmem_aligned_inner(size_t alignment, size_t size) {
        if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
            return NULL;
        }
        alignment = sizeof(void*) > alignment ? sizeof(void*) : alignment;
        void* ptr = NULL;
        if(posix_memalign(&ptr, alignment, size) != 0) {
            return NULL;
        }
        return ptr;
    }
#endif
// clang-format on

#if !defined(XMEM_DEFINED)
    #error "No supported aligned allocation backend found for this platform/standard."
#endif

static xmem_alloc_fn   s_xmem_alloc_fn   = XMEM_ALLOC_INNER;
static xmem_aligned_fn s_xmem_aligned_fn = XMEM_ALIGNED_INNER;
static xmem_free_fn    s_xmem_free_fn    = XMEM_FREE_INNER;

void* xmem_alloc(size_t size) {
    return s_xmem_alloc_fn(size);
}

void* xmem_aligned(size_t alignment, size_t size) {
    return s_xmem_aligned_fn(alignment, size);
}

void xmem_free(void* ptr) {
    s_xmem_free_fn(ptr);
}

int xmem_override(xmem_alloc_fn alloc, xmem_aligned_fn aligned, xmem_free_fn free) {
    if(alloc == NULL || aligned == NULL || free == NULL) {
        return -1;
    }
    s_xmem_alloc_fn   = alloc;
    s_xmem_aligned_fn = aligned;
    s_xmem_free_fn    = free;
    return 0;
}
