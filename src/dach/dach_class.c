#include "dach/dach_class.h"
#include <assert.h>
#include <stdlib.h>


void *dach_new(const void * class_, const size_t argc, const void **argv) {
  const Class *class = class_;
  void *p = calloc(1, class->size);
  assert(p);
  *(const Class **) p = class;
  assert(class->ctor);
  if (class->ctor) {
    p = class->ctor(p, argc, argv);
  }
  return p;
}

void dach_delete(void *self) {
  assert(self);
  const Class **cp = self;
  if (self && * cp && (*cp)->dtor) {
    self = (*cp)->dtor(self);
  }
  free(self);
}

int dach_differ(const void * self, const void * b) {
  const Class * const *cp = self;
  assert(self && * cp && (*cp)->differ);
  return (*cp)->differ(self, b);
}

