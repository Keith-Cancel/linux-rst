#ifndef XMEM_ALLOC_WRAP_HEADER_H
#define XMEM_ALLOC_WRAP_HEADER_H

#include "types.h"

typedef void* (*xmem_alloc_fn)(usz size);
typedef void* (*xmem_aligned_fn)(usz alignment, usz size);
typedef void  (*xmem_free_fn)(void* ptr);

void* xmem_alloc(usz size);
void* xmem_aligned(usz alignment, usz size);
void  xmem_free(void* ptr);

int xmem_override(xmem_alloc_fn alloc, xmem_aligned_fn aligned, xmem_free_fn free);

#endif
