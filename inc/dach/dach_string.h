#ifndef DACH_STRING_H
#define DACH_STRING_H
/*
 * We could give this options at some point to make it relative etc ie 
 * RELATIVE | 0
 */
struct substr {
  unsigned int pos;
  unsigned int len;
};

unsigned int dach_find_prev_substr(const unsigned char *slab, const unsigned size, const unsigned int pos, const unsigned int dist, struct substr *sub);
#endif /* HSTRING_H */
