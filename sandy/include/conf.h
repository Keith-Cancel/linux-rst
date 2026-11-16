#ifndef _CONFIG_FILE_H
#define _CONFIG_FILE_H

#include "verbose.h"

typedef struct con_file_s ConFile;

ConFile* con_file_new(const char* path, Verbose verbose);
void     con_file_free(ConFile* file);

// Returns NULL when there are no more lines.
const uint8_t *con_file_next_line(ConFile *cf);

#endif