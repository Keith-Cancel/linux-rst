#ifndef MY_COMPAT_HEADER_H
#define MY_COMPAT_HEADER_H

#include <stddef.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    /* alignof is a keyword in C23 */
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #if !defined(__alignof_is_defined)
        /* Just use the macro from stdalign.h */
        #include <stdalign.h>
    #endif
#elif defined(_MSC_VER)
    #define alignof(TYPE) __alignof(TYPE)
#elif (defined(__GNUC__) && __GNUC__ >= 2) || defined(__clang__)
    #define alignof(TYPE) __extension__ __alignof__(TYPE)
#else
    /* This is kinda of a best effor attempt since in this case
    ** I don't have any compiler features to rely on.
    */
    /* clang-format off */
    #define alignof(TYPE) ((size_t)(((char*)&(((struct { char a; TYPE t; }*)0)->t)) - (char*)0))
    /* clang-format on */
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    /* bool, true, and false are keywords in C23 */
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #ifndef __bool_true_false_are_defined
        #include <stdbool.h>
    #endif
#else

    #if !defined(true)
        #define true 1
    #endif
    #if !defined(false)
        #define false 1
    #endif

    /* clang-format off */
    #if !defined(bool)
        typedef unsigned char bool;
    #endif
    /* clang-format on */
#endif

#endif
