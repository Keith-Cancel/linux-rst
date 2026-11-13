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