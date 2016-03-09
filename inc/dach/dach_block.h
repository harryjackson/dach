#ifndef dach_block_H
#define dach_block_H
#include "dach_types.h"
#include <stdio.h>

typedef enum {
  READER           = 0,
  WRITER           = 1 << 1,
} DACH_BLOCK_FLAGS;


/*
 * (dach_block_init.length <= 0) -> C.R.E,
 * for type size_t this really means don't specify a
 * block length of zero or we'll blow up.
 * Violations of the other types are C.R.E ie mode that's not
 * valid etc.
 */
typedef struct dach_block_init {
  size_t length;
  u32 bid;
  DACH_STREAM_MODE mode;
  void *data; //Read mode start position
} dach_block_init;

typedef struct dach_block     dach_block;

typedef struct dach_block_ops {
  ssize_t (*read          )(dach_block *block,       void *buf, size_t count);
  ssize_t (*write         )(dach_block *block, const void *buf, size_t count);
  ssize_t (*tofile        )(dach_block *block, FILE *fd);
  size_t  (*bytes_written )(dach_block *block);
  u32     (*id            )(dach_block *block);
  size_t  (*length        )(dach_block *block);
  u8      (*get_c         )(dach_block *block);
  size_t  (*seek          )(dach_block *block, size_t pos);
} dach_block_ops;

struct dach_block {
  void           *obj;
  dach_block_ops *ops;
};

dach_block *dach_block_new(dach_block_init *dbi);
void        dach_block_free(dach_block *block);

#endif /* dach_block_H */
