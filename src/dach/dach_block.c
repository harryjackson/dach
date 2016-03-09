#include "dach/dach_block.h"
#include "dach/dach_mem.h"
#include "dach/dach_types.h"
#include "dach/dach_error.h"
#include <ctype.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>

#define DACH_M_VOID_TO_BLOCK  \
dach_block_obj  *obj  = (dach_block_obj*)block->obj;   \
assert(obj->err_no >= 0); \
DACH_OBJ_VALID(obj->err_no);

#define DACH_M_BLOCK_THREAD_ASSERT  \
assert(obj->thread_id == pthread_self());

static u32     block_id            (dach_block *block);
static ssize_t block_read          (dach_block *block,       void *buf, size_t count);
static ssize_t block_write         (dach_block *block, const void *buf, size_t count);
static ssize_t block_tofile        (dach_block *block, FILE *fd);
static size_t  block_bytes_written (dach_block *block);
static size_t  block_length        (dach_block *block);
static u8      block_getc          (dach_block *block);
static size_t  block_seek          (dach_block *block, size_t pos);

static dach_block_ops b_ops = {
  .read          = block_read,
  .write         = block_write,
  .tofile        = block_tofile,
  .bytes_written = block_bytes_written,
  .id            = block_id,
  .length        = block_length,
  .get_c         = block_getc,
  .seek          = block_seek,
};

/*
 * Block
 * size   == bytes allocated
 * length == bytes used in the block ie can be used in a loop. In Go parlance I should call this
 * range
 */
typedef struct dach_block_obj {
  int              type;
  u32              id;
  DACH_STREAM_MODE mode;
  u32              free;
  size_t           pos;
  size_t           bytes_written;
  size_t           start_pos;
  u8               *data;
  size_t           size;        // Memory allocated to data
  size_t           length;      // Length requested
  pthread_t        thread_id; //invariant (only one thread should be able to work on a block)
  int              err_no;
  int              pad;
} dach_block_obj;


typedef struct dach_block_impl {
  dach_block_obj *obj;
  dach_block_ops *ops;
} dach_block_impl;


static void priv_block_init(dach_block_impl *block, dach_block_init *dbi);



dach_block *dach_block_new(dach_block_init *dbi) {
  assert(dbi->length >= 0);
  assert(dbi->data == NULL || dbi->mode == DACH_STREAM_READABLE);
  dach_block_impl *db_impl;

  DACH_NEW(db_impl);
  DACH_NEW(db_impl->obj);

  db_impl->ops = &b_ops;
  priv_block_init(db_impl, dbi);
  return (dach_block*)db_impl;
}

void dach_block_free(dach_block *block) {
  dach_block_obj *db_obj = (dach_block_obj*)block->obj;
  if(db_obj->mode == DACH_STREAM_WRITABLE) {
    DACH_FREE(db_obj->data);
  }
  DACH_FREE(db_obj);
  DACH_FREE(block);
}

static void priv_block_init(dach_block_impl *db_impl, dach_block_init *dbi) {
  assert(db_impl->obj != NULL);
  dach_block_obj *obj = (dach_block_obj *)db_impl->obj;

  obj->thread_id = pthread_self();
  obj->id        = dbi->bid;
  obj->mode      = dbi->mode;
  obj->length    = dbi->length;
  obj->size      = dbi->length;
  obj->free      = 0;
  obj->start_pos = 0;
  obj->err_no    = 0;
  obj->pos       = 0;

  if(dbi->mode == DACH_STREAM_WRITABLE) {
    //printf("length == %lu\n", obj->length);
    obj->data = NULL;
    obj->data = malloc(1 * obj->size);
    assert(obj->data);
  }
  else {
    //Feels dangerous to do this rather than
    //copy... is copy slow?
    obj->data = dbi->data;
  }
}

static ssize_t block_read(dach_block *block, void *buf, size_t count) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  //fprintf(stderr, "bid=%u wpos=%lu wtf count=%lu\n", obj->id, obj->write_pos, count);
  assert(obj->pos <= obj->length);
  if ((count + obj->pos) >= obj->length) {
    //Adjust count to remaining bytes to be read.
    count = obj->length - obj->pos;
    if(count == 0) {
      return -1;
    }
  }
  size_t i = 0;
  u8 *tbuf = buf;
  size_t bytes_written = 0;
  for(i = 0; i < count; i++) {
    //const char *ch = tmp[i];
    if(obj->pos == obj->length) {
      break;
    }
    tbuf[i] = obj->data[obj->pos];
    obj->pos++;
    bytes_written++;

  }

  assert(obj->pos <= obj->length);
  if(obj->pos > obj->length) {
    fprintf(stderr, "bid=%u wpos=%lu count=%lu\n", obj->id, obj->pos, count);
    assert(NULL);
  }
  return bytes_written;
}

static ssize_t block_write(dach_block *block, const void *buf, const size_t count) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  assert(buf != NULL);
  assert(count > 0);

  assert(obj->mode == DACH_STREAM_WRITABLE);
  assert(obj->data);
  const u8 *tmp = buf;
  //assert(tmp[0]);
  if ((count + obj->pos) >= obj->length) {
    //printf("realloc length= %lu\n",obj->length);
    obj->data = realloc(obj->data, count + obj->pos);
    assert(obj->data);
    obj->length = count + obj->pos;
    //printf("realloc bid=%u wpos=%lu count=%lu length=%lu\n", obj->id, obj->pos, count, obj->length);
  }

  size_t i = 0;
  for(i = 0; i < count; i++) {
    //fprintf(stderr, "bid=%u wpos=%lu count=%lu\n", obj->id, obj->pos, count);
    obj->data[obj->pos] = (u8) tmp[i];
    obj->pos++;
    if (obj->pos > obj->length) {
      obj->length = obj->pos;
      assert(obj->pos == obj->length);
    }
    obj->bytes_written++;
  }

  assert(obj->pos <= obj->length);
  if(obj->pos > obj->length) {
    fprintf(stderr, "bid=%u wpos=%lu count=%lu\n", obj->id, obj->pos, count);
  }
  return 1;
}

static u32 block_id(dach_block *block) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  //assert(NULL);
  return obj->id;
}

static ssize_t block_tofile(dach_block *block, FILE *fd) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  assert(fd != NULL);
  size_t i = 0;
  ssize_t written = 0;
  //for(i = 0; i < obj->length; i++) {
  written = fwrite(obj->data, 1, obj->length, fd);
  assert(written >= 0);
  assert((size_t)written == obj->length);
  //}
  //assert(NULL);
  return written;
}

static size_t block_length(dach_block *block) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  //assert(obj->length < 4194967296);
  return obj->length;
}

/* Exported as an Operation */
static u8 block_getc(dach_block *block) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  //assert(NULL != NULL);
  return obj->data[obj->pos++];
}

static size_t block_bytes_written(dach_block *block) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  return obj->bytes_written;
}

/*
 Operation: Seeking outside the boundaires of 
 the block as allocated is an error
 */
static size_t block_seek(dach_block *block, size_t pos) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;

  assert(pos <= obj->length);
  obj->pos = 0;
  /*if(pos > obj->lensize) {
    obj->write_pos = obj->size;
    return obj->write_pos;
  }
  obj->write_pos = pos;
   */
  return obj->pos;
}

#undef DACH_M_VOID_TO_BLOCK
#undef DACH_M_BLOCK_THREAD_ASSERT


