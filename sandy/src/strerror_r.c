// Make GNU Source is not defined so we always XSI version
#undef _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L

#include <string.h>
#include <stdint.h>

int strerror_r_wrap(int errnum, char *buf, size_t buflen) {
    return strerror_r(errnum, buf, buflen);
}