#include "verbose.h"
#include <stdio.h>
#include <string.h>


#if defined(__GNUC__) && !defined(__clang__)
    __attribute__((optimize("Os")))
#elif defined(__clang__)
    __attribute__((minsize))
#endif
void verbose_header(Verbose v, const char* title) {
    if(!verbose_enabled(v)) {
        return;
    }
    size_t len = strlen(title);
    if (len > 34) {
        len = 34; // keep at least one '=' on each side
    }
    size_t pad  = 38 - (len + 2); // Add 2 for the spaces on each side.
    size_t lpad = (pad + 1) / 2;
    size_t rpad = pad - lpad;

    // 38 + 8(color) + NL = 47
    uint8_t buff[48];
    memset(buff, '=', 48);

    bool color = verbose_color(v);
    // Start after the left '=' pad and walk the pos pointer.
    uint8_t* pos = buff + lpad;
    // Pad left with space.
    *pos = ' ';
    pos += 1;
    // Add bold if color is enabled.
    if(color) {
        memcpy(pos, &(uint8_t[4]) { 0x1b, '[', '1', 'm' }, 4);
        pos += 4;
    }
    // Copy The 
    memcpy(pos, title, len);
    pos += len;
    // Pad right with space
    *pos = ' ';
    pos += 1;
    if(color) {
        memcpy(pos, &(uint8_t[4]) { 0x1b, '[', '0', 'm' }, 4);
        pos += 4;
    }
    pos += rpad;
    *pos = '\n';
    
    size_t amt = (pos + 1) - buff;
    fwrite(buff, 1, amt, stdout);
}

#define BUFF_SZ  128
#define T_MAX    32
#define TS_MAX   (6 + 1 + 11 + T_MAX + 2)
#define VAL_MAX  (BUFF_SZ - (TS_MAX + 1))
_Static_assert(BUFF_SZ >= TS_MAX, "Buffer too small for title and status");
_Static_assert(VAL_MAX >= 32, "Buffer is too small");


#if defined(__GNUC__) && !defined(__clang__)
    __attribute__((optimize("Os")))
#elif defined(__clang__)
    __attribute__((minsize))
#endif
void verbose_line(Verbose v, const char* title, const char* value, VStatus stat) {
    const uint8_t OK[4]    = { ' ', 'O', 'K', ' ' };
    const uint8_t FAIL[4]  = { 'F', 'A', 'I', 'L' };
    const uint8_t RED[7]   = { 0x1b, '[', '1', ';', '3', '1', 'm' };
    const uint8_t GREEN[7] = { 0x1b, '[', '1', ';', '3', '2', 'm' };

    if(!verbose_enabled(v)) {
        return;
    }
    title = title ? title : "(null)";
    value = value ? value : "(null)";

    bool   color = verbose_color(v);
    size_t twid  = verbose_title_width(v);
    if(twid > T_MAX) {
        twid = T_MAX;
    }
    // 6 (status) + 1 (SP) + 32 (Title Len Max) + 2 (SP Colon) = 41
    // ANSI Control 7 (Color + Bold) + 4 (Reset) = 11
    // 41 + 11 = 52 for status and title.
    uint8_t buff[BUFF_SZ];
    memset(buff, ' ', TS_MAX > 64 ? TS_MAX : 64);

    // Begin a pointer walk.
    uint8_t* pos = buff;
    // Print status if it's either V_OK or V_FAIL
    if(stat == V_OK || stat == V_FAIL) {
        bool good = stat == V_OK;

        *pos = '[';
        pos += 1;

        if(color) {
            memcpy(pos, good ? GREEN : RED, 7);
            pos += 7;
        }

        memcpy(pos, good ? OK : FAIL, 4);
        pos += 4;

        if(color) {
            memcpy(pos, &(uint8_t[4]) { 0x1b, '[', '0', 'm' }, 4);
            pos += 4;
        }

        memcpy(pos, "] ", 2);
        pos += 2;
    } else {
        pos += 7;
    }

    size_t i = 0;
    for(; i < twid; i++) {
        char c = title[i];
        if(c == '\0') {
            break;
        }
        *pos = c;
        pos += 1;
    }
    *pos = ':';
    pos += (twid + 2) - i;

    // Add 1 so there is room for new line.
    size_t free = BUFF_SZ - ((pos + 1) - buff);
    for(size_t j = 0; j < free; j++) {
        char c = value[j];
        if(c == '\0') {
            break;
        }
        *pos = c;
        pos += 1;
    }

    *pos = '\n';
    size_t amt = (pos + 1) - buff;
    fwrite(buff, 1, amt, stdout);
}
