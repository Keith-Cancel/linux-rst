#ifndef XMEM_ALLOC_WRAP_HEADER_H
#define XMEM_ALLOC_WRAP_HEADER_H

#include <stddef.h>

typedef void* (*xmem_alloc_fn)(size_t size);
typedef void* (*xmem_aligned_fn)(size_t alignment, size_t size);
typedef void  (*xmem_free_fn)(void* ptr);

void* xmem_alloc(size_t size);
void* xmem_aligned(size_t alignment, size_t size);
void  xmem_free(void* ptr);

void xmem_override(xmem_alloc_fn alloc, xmem_aligned_fn aligned, xmem_free_fn free);

#endif
