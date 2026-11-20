#ifndef _UTF8_UTIL_FILE_H
#define _UTF8_UTIL_FILE_H

#include <stdbit.h>

#if defined(__BMI2__)
#include <immintrin.h>

static inline uint32_t utf8_decode_unchecked(uint32_t utf8, unsigned len) {
    static const uint32_t PEXT_TBL[5] = {
        0x00000000,
        0x7f000000, // 0xxxxxxx
        0x1f3f0000, // 110xxxxx 10xxxxxx
        0x0f3f3f00, // 1110xxxx 10xxxxxx 10xxxxxx
        0x073f3f3f, // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    };
    uint32_t msk = PEXT_TBL[len];
    return _pext_u32(utf8, msk);
}
#else

static inline uint32_t utf8_decode_unchecked(uint32_t utf8, unsigned len) {
    uint32_t msk = (0xffU >> len); // Mask for prefix bits
    uint32_t shf = 6 * (4 - len);  // Final Shift amount.
    uint32_t cpt = utf8 & 0x3f;
    cpt |= ((utf8 >> 8)  & 0x3f) << 6;
    cpt |= ((utf8 >> 16) & 0x3f) << 12;
    cpt |= ((utf8 >> 24) & msk)  << 18;
    cpt >>= shf;
    return cpt;
}

#endif

// Always loads 4 bytes, handling byte order.
static inline uint32_t utf8_load_padded(const uint8_t* data) {
    uint32_t chunk;
    memcpy(&chunk, data, 4);
    #if __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__
        chunk = __builtin_bswap32(chunk);
    #elif __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__
    #else
        #error "Mixed or unsupported endianness"  
    #endif
    return chunk;
}

static inline unsigned utf8_len(uint32_t utf8) {
    unsigned lz = __builtin_clz(~utf8);
    return lz + (lz == 0);
}

static inline uint32_t utf8_next_padded_unchecked(const uint8_t** data) {
    uint32_t utf8 = utf8_load_padded(*data);
    unsigned len  = utf8_len(utf8);
    uint32_t cpt  = utf8_decode_unchecked(utf8, len);
    *data += len;
    return cpt;
}

#endif