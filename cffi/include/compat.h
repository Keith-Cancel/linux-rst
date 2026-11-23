#ifndef MY_COMPAT_HEADER_H
#define MY_COMPAT_HEADER_H

#include <stddef.h>

/* clang-format off */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
   /* alignof is a keyword in C23 */
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    /* Just use the macro from stdalign.h */
    #include <stdalign.h>
#elif defined(_MSC_VER)
    #define alignof(TYPE) __alignof(TYPE)
#elif (defined(__GNUC__) && __GNUC__ >= 2) || defined(__clang__)
    #define alignof(TYPE) __extension__ __alignof__(TYPE)
#else
    #define alignof(TYPE) ((size_t)(((char *)&(((struct { char a; TYPE b; } *)0)->b)) - (char *)0))
#endif
/* clang-format on */

#endif
