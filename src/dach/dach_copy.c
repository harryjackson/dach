#include "dach/dach_copy.h"
#include <assert.h>

int dach_copy(dach_block *in, dach_block *out) {
  assert(in  != NULL);
  assert(out != NULL);
  size_t pos = 0;
  size_t length = in->ops->length(in);
  //printf("bsize = %zu bcount=%u\n", bsize, bcount);
  for(pos = 0; pos < length; pos++) {
    char c = in->ops->get_c(in);
    //printf("char == %c\n", c);
    out->ops->write(out, &c, 1);
  }
  return 0;
}