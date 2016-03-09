#include "dach/dach_string.h"

unsigned int dach_find_prev_substr(const unsigned char *slab, const unsigned size, const unsigned int pos, const unsigned int dist, struct substr *sub) {
  sub->pos = 0;
  sub->len = 0;
  unsigned int b, f; // back, front
  f = pos; 
  if(dist > pos) {
    b = 0;// start at very first element of slab
  }
  else {
    b = pos - dist;
  }
  unsigned int n = 0;
  unsigned int slide = b;
  while(b < pos) {
    n = 0;
    while((b < pos) && (f < size) && (slab[b] == slab[f])) {
      //printf("\n%c-%c\n", slab[b], slab[f]);
      b++;
      f++;
      n++;
    }
    if(n > sub->len) {
      sub->len = n;
      sub->pos = slide;
    }
    b = (++slide);
    f = pos;
  }
  return 0;
}
