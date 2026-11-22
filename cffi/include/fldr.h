#ifndef FILE_LOADER_H
#define FILE_LOADER_H

typedef struct file_loader_s Fldr_t;

Fldr_t* fldr_new();
void    fldr_free(Fldr_t* ldr);

#endif
