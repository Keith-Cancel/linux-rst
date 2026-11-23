#ifndef FLEXIBLE_ARRAY_TYPE_H
#define FLEXIBLE_ARRAY_TYPE_H

#include <stddef.h>

void* flex_arr_new(size_t capacity, size_t el_size, size_t el_align);
void* flex_arr_expand(void* arr);
void  flex_arr_free(void* arr);

#endif
