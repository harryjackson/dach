#include "dach/dach_mem.h"
#include <assert.h>

void *dach_mem_calloc(long count, long nbytes, const char *file, int line) {
  void *ptr;
  assert(count > 0); 
  assert(nbytes > 0);
  //printf("count=%zu nbytes = %zu\n", count, nbytes);
  ptr = calloc(count, nbytes);
  assert(ptr != NULL);
  return ptr;
}

void dach_mem_free(void *ptr, const char *file, int line) {
  assert(ptr);
  if(ptr) {
    free(ptr);
  }
  ptr = 0;
}

void *dach_mem_realloc(void *ptr, long nbytes, const char *file, int line) {
  assert(ptr);
  assert(nbytes > 0); 
  ptr = realloc(ptr, nbytes);
  assert(ptr != NULL);
  return ptr;
}

