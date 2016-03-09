
#include <stdarg.h>
#include <stdio.h>


void   *dach_new(const void *class_, const size_t argc, const void **argv);
void   dach_delete(void *self);

void   *dach_clone(const void *self);
int    dach_differ(const void *self, const void *b);
size_t dach_sizeOf(const void *self);

typedef struct Class {
  size_t size;
  void * (*ctor)   (void *self, const size_t argc, const void **argv);
  void * (*dtor)   (void *self);
  void * (*clone)  (const void *self);
  int    (*differ) (const void *self, const void *b);
} Class;
