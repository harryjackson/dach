#ifndef DACH_MEM_H
#define DACH_MEM_H
#include <stdlib.h>

void *dach_mem_calloc(long count, long nbytes, const char *file, int line);
void *dach_mem_realloc(void *ptr, long nbytes, const char *file, int line);
void  dach_mem_free(void *ptr, const char *file, int line);
#define DACH_CALLOC(count, nbytes)  dach_mem_calloc((count), (nbytes), __FILE__, __LINE__)

#define DACH_N_NEW(count, p) ((p) = DACH_CALLOC(count, (long) sizeof(*(p)) ))
#define DACH_NEW(p)          ((p) = DACH_CALLOC(1    , (long) sizeof(*(p)) ))

#define DACH_FREE(ptr) ((void)(dach_mem_free((ptr), __FILE__, __LINE__), (ptr) = 0))
#define DACH_REALLOC(ptr, nbytes)   ((ptr) = dach_mem_realloc((ptr), (nbytes), __FILE__, __LINE__))
#endif /* H_MEM_H */
