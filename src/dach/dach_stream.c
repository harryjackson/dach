#include "dach/dach_stream.h"
#include "dach/dach_error.h"
#include "dach/dach_header.h"
#include "dach/dach_io.h"
#include "dach/dach_log.h"
#include "dach/dach_mem.h"
#include <apr-1/apr_thread_pool.h>
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define DACH_M_VOID_TO_STREAM                                  \
  dach_stream_impl *dos        = (dach_stream_impl *)stream;   \
  dach_stream_obj  *stream_obj = (dach_stream_obj  *)dos->obj; \
  assert(stream_obj->err_no == 0);                               \
  DACH_OBJ_VALID(stream_obj->err_no);

typedef struct dblock {
  dach_block * obj;
  u32          id;
  u32          written_to_stream;
} dblock;

typedef struct sstream {
  dach_stream * obj;
  dach_stream * next;
  u32           order;
} sstream;

/* Stream V1 Ops */
static dach_block_ops *block_ops      (dach_stream *stream);
static size_t          blocks         (dach_stream *stream);
static dach_block*     new_block      (dach_stream *stream);
static dach_block*     new_write_block(dach_stream *stream, dach_block *block);
static void            free_block     (dach_stream *stream, dach_block *block);
static int             err            (dach_stream *stream);

static dach_stream_ops opv1 = {
  block_ops,
  blocks,
  new_block,
  new_write_block,
  free_block,
  err,
};


typedef struct dach_stream_obj {
  DACH_TYPE        type;
  DACH_STREAM_MODE mode;
  size_t wpos; // Read Position
  size_t rpos; // Write Position
  const char * filename;
  const char * outfile_name;
  FILE       * outfile_stream;
  FILE       * tmpfile;
  off_t        filesize;
  off_t        out_filesize;
  size_t       mmap_data_size;
  u8         * mmap_data;
  int          mmap_fd;
  size_t       block_count;
  size_t       blocks_returned;
  size_t block_size_hint;
  size_t block_size_actual;
  size_t block_stream_order;
  size_t last_block_size;
  dblock * dblock_array;
  dblock * dblock_array_out;
  int hdr_size;
  int err_no;
} dach_stream_obj;

typedef struct dach_stream_impl {
  dach_stream_obj * obj;
  dach_stream_ops * op;
} dach_stream_impl;


typedef struct dach_operation_obj {
  DACH_TYPE           type;
  DACH_STREAM_MODE    mode;
  dach_block        * in_blk;
  dach_block        * out_blk;
} dach_operation_obj;


typedef struct dach_operation_impl {
  dach_operation_obj * obj;
  dach_operation_ops * ops;
} dach_operation_impl;

typedef struct dach_pipe_obj {
  pthread_mutex_t       stream_array_mutex;
  dach_stream_impl    * source;
  size_t                oper_count;
  dach_operation_impl * oper_array;
} dach_pipe_obj;

typedef struct dach_pipe_impl {
  dach_pipe_obj * obj;
  dach_pipe_ops * ops;
} dach_pipe_impl;

static int block_written(dach_stream_obj *stream_obj, u32 bid);
static void write_to_stream(dach_stream_obj *stream_obj, u32 bid);
static size_t get_block_count(u64 infilesize, size_t block_size);
static size_t get_last_block_size(u64 infilesize, size_t block_size);
static int test_block_size(dach_stream_obj *stream_obj);
static void lazy_create_blocks(dach_stream_obj *stream_obj);
static void mmap_in_file(dach_stream_obj *stream_obj);
//static void mmap_out_file(dach_stream_obj *stream_obj);
dach_stream *priv_file_reader_new(DACH_CONFIG *dc);
dach_stream *stream2_new(DACH_CONFIG *dc);

static ssize_t dach_pipe_wait(dach_pipe *dpipe);
static ssize_t dach_pipe_start(dach_pipe *dpipe);
static ssize_t dach_pipe_tofile(dach_pipe *dpipe, FILE *fd);
static ssize_t dach_pipe_add(dach_pipe *dpipe, dach_operation *dop);
static ssize_t dach_operation_read(dach_operation *d, void *buf, size_t count);
static ssize_t dach_operation_write(dach_operation *d, const void *buf, size_t count);


dach_operation *dach_operation_new(dach_operation_callback callback) {
  dach_operation_ops  *dop_ops  = NULL;
  dach_operation_impl *dop_impl = NULL;
  assert(callback);

  DACH_NEW(dop_impl);
  DACH_NEW(dop_impl->obj);
  DACH_NEW(dop_impl->ops);

  dop_impl->ops->operation = callback;
  return (dach_operation*)dop_impl;
}

void dach_operation_free(dach_operation *ptr) {
  dach_operation_impl *dop_impl = (dach_operation_impl*)ptr;
  DACH_FREE(dop_impl->obj);
  DACH_FREE(dop_impl->ops);
  DACH_FREE(dop_impl);
}


dach_pipe *dach_pipe_new(dach_stream *ds, dach_operation *dop) {
  assert(ds  != NULL);
  assert(dop != NULL);
  dach_pipe_impl      *pip_impl;
  dach_stream_impl    *source = (dach_stream_impl *)ds;

  DACH_NEW(pip_impl);
  DACH_NEW(pip_impl->obj);
  DACH_NEW(pip_impl->ops);
  DACH_N_NEW(16,pip_impl->obj->oper_array);

  pip_impl->ops->add        = dach_pipe_add;
  pip_impl->ops->start      = dach_pipe_start;
  pip_impl->ops->tofile     = dach_pipe_tofile;
  pip_impl->ops->wait       = dach_pipe_wait;
  
  pip_impl->obj->source     = source;
  dach_operation_impl *dop_impl;

  dop_impl                      = (dach_operation_impl*)dop;
  dop_impl->obj->type           = DACH_TYPE_OPERATION;
  dop_impl->obj->mode           = DACH_STREAM_READWRITE;

  pip_impl->obj->oper_array[pip_impl->obj->oper_count]  = *dop_impl;
  pip_impl->obj->oper_count++;
  assert(pip_impl->obj->oper_count == 1);
  return (dach_pipe*)pip_impl;
}


void dach_pipe_free(dach_pipe *pip) {
  assert(pip != NULL);
  dach_pipe_impl *pip_impl = (dach_pipe_impl*)pip;
  DACH_FREE(pip_impl->obj->oper_array);
  DACH_FREE(pip_impl->obj);
  DACH_FREE(pip_impl->ops);
  DACH_FREE(pip_impl);
}

static void dach_thread_pipe(dach_stream *dstream, dach_operation_impl *dop_impl, size_t bcount) {

  dop_impl->ops->read  = &dach_operation_read;
  dop_impl->ops->write = &dach_operation_write;
  dach_operation_callback callback = dop_impl->ops->operation;
  assert(dop_impl->ops->operation);

  size_t bid = 0;
  for(bid = 0; bid < bcount; bid++) {
    dach_block *in_blk  = dstream->ops->new_block(dstream);
    dach_block *out_blk = dstream->ops->write_block(dstream, in_blk);
    assert(out_blk);
    dop_impl->obj->in_blk  = in_blk;
    dop_impl->obj->out_blk = out_blk;
    callback((dach_operation*)dop_impl);
  }
}


static ssize_t dach_pipe_start(dach_pipe *pip) {
  dach_pipe_impl   *pip_impl    = (dach_pipe_impl*)pip;
  dach_stream      *dstream     = (dach_stream*)pip_impl->obj->source;
  dach_stream_impl *stream_impl = pip_impl->obj->source;
  //printf("bsize = %zu bcount=%u\n", bsize, bcount);
  size_t op_count = pip_impl->obj->oper_count;
  size_t i = 0;

  /*
   * \todo
   * Need to make this work for more than one operation before
   * threading it. Once a stream source has been read and the 
   * blocks written we need to write a new
   */
  for(i = 0; i < op_count; i++) {
    dach_operation_impl dop_impl  = pip_impl->obj->oper_array[i];
    u32 bcount                    = stream_impl->op->blocks(dstream);
    dach_thread_pipe(dstream, &dop_impl, bcount);
  }
  return 0;
}


static ssize_t dach_pipe_wait(dach_pipe *dpipe) {
  return -1;
}

static ssize_t dach_pipe_add(dach_pipe *pyp, dach_operation *dop) {
  dach_pipe_impl      *pyp_impl = (dach_pipe_impl*)pyp;
  dach_operation_impl *dop_impl = (dach_operation_impl*)dop;
  assert(pyp_impl->obj->oper_count < 16);
  dop_impl                      = (dach_operation_impl*)dop;
  dop_impl->obj->type           = DACH_TYPE_OPERATION;
  dop_impl->obj->mode           = DACH_STREAM_READWRITE;

  pyp_impl->obj->oper_array[pyp_impl->obj->oper_count]  = *dop_impl;
  pyp_impl->obj->oper_count++;
  assert(pyp_impl->obj->oper_count <= 16);
  return -1;
}

static ssize_t dach_operation_read(dach_operation *dop, void *buf, size_t count) {
  assert(dop);
  assert(buf);
  dach_operation_impl *opaq_op = (dach_operation_impl*)dop;
  dach_block *in          = opaq_op->obj->in_blk;
  ssize_t r               = in->ops->read(in, buf, count);
  return r;
}

static ssize_t dach_operation_write(dach_operation *dop, const void *buf, size_t count) {
  assert(dop);
  assert(buf);
  assert(count > 0);
  dach_operation_impl *dop_impl = (dach_operation_impl*)dop;
  const u8 *ibuf = (const u8*)buf;
  size_t i = 0;
  dach_block *out_blk = dop_impl->obj->out_blk;
  i = out_blk->ops->write(out_blk, buf, count);
  return (ssize_t) i;
}


static ssize_t dach_pipe_tofile(dach_pipe *dpipe, FILE *fd) {
  dach_pipe_impl   *pip_impl    = (dach_pipe_impl *)dpipe;
  dach_stream_impl *stream_impl = pip_impl->obj->source;

  size_t bcount = stream_impl->obj->block_count;
  size_t i = 0;
  for(i = 0 ; i < bcount; i++) {
    dblock db    = stream_impl->obj->dblock_array_out[i];
    db.obj->ops->seek(db.obj, 0);
    size_t wrote = db.obj->ops->tofile(db.obj, fd);
    assert(wrote == db.obj->ops->length(db.obj));
  }
  return -1;
}

dach_stream *dach_stream_new(DACH_CONFIG *dc) {
  //assert(dc->file != NULL);
  return priv_file_reader_new(dc);
}

dach_stream *priv_file_reader_new(DACH_CONFIG *dc) {
  const char *filename = dc->file;
  const char *mode_str = dc->mode;
  int valid_mode = ((strcmp(mode_str,"r") != 0) || (strcmp(mode_str,"w") != 0));
  DACH_STREAM_MODE mode = DACH_STREAM_INVALID; //Invalid
  DACH_TYPE        type = DACH_TYPE_STREAM;
  if (strcmp(mode_str, "w") == 0) {
    //printf("writable\n");
    mode = DACH_STREAM_WRITABLE;
  }
  else if (strcmp(mode_str, "r") == 0) {
    //printf("readable %s\n", mode_str);
    mode = DACH_STREAM_READABLE;
  }
  else {
    mode = DACH_STREAM_INVALID;
    dach_perror(DACH_LOG_FATAL, "%s", "Invalid stream mode");
    assert(NULL);// NULL;
  }
  assert(mode != DACH_STREAM_INVALID);
  dach_stream_impl *stream_impl;
  dach_stream_obj *stream_obj;
  DACH_NEW(stream_impl);
  DACH_NEW(stream_obj);
  stream_obj->type              = type;
  stream_obj->mode              = mode;
  stream_obj->err_no            = 0;
  stream_obj->hdr_size          = dach_header_size();
  stream_obj->filename          = filename;


  if(stream_obj->mode == DACH_STREAM_READABLE) {
    off_t filesize              = dach_io_file_size(filename);
    assert(filesize > 0);
    stream_obj->filesize          = filesize;
    stream_obj->mmap_data_size    = filesize;

    stream_obj->blocks_returned   = 0;
    stream_obj->block_count       = get_block_count(filesize, dc->block_size);
    stream_obj->block_size_hint   = dc->block_size;
    stream_obj->block_size_actual = dc->block_size;

    stream_obj->last_block_size   = get_last_block_size(filesize, stream_obj->block_size_actual);
    test_block_size(stream_obj);
    stream_obj->outfile_name      = filename;

    mmap_in_file(stream_obj);
    lazy_create_blocks(stream_obj);
  }
  else if (stream_obj->mode == DACH_STREAM_WRITABLE) {
    assert(NULL);
  }
  stream_impl->op = &opv1;
  stream_impl->obj = stream_obj;

  return (dach_stream*) stream_impl;
}

/*
static void set_last_block_size(dach_stream_obj *stream_obj, u64 infilesize) {
  * The last block length differs from the rest unless the input filesize is a multiple
   * of the block size.
   *
  dach_block *last_block = stream_obj->block_array[stream_obj->block_count - 1];
  DACH_BLOCK *lstblock = last_block->priv;
  assert(lstblock->length == stream_obj->block_size_actual);
  lstblock->length = get_last_block_size(infilesize, stream_obj->block_size_actual);
}*/

static void lazy_create_blocks(dach_stream_obj *stream_obj) {
  dblock *dblk;
  DACH_STREAM_MODE mode = stream_obj->mode;
  /* The last block is of variable size so allocating a full block is not actually 
   necessary but simpler for now.
   */
  stream_obj->dblock_array     =  malloc(sizeof(*dblk) * stream_obj->block_count);
  stream_obj->dblock_array_out =  malloc(sizeof(*dblk) * stream_obj->block_count);
  assert(stream_obj->dblock_array     != NULL);
  assert(stream_obj->dblock_array_out != NULL);

  u32 bid = 0;
  size_t pos_pointer = 0;
  const size_t block_count = stream_obj->block_count;
  const size_t last_bid    = stream_obj->block_count - 1;

  for(bid = 0; bid < block_count; bid++) {
    dach_block_init dbin;
    dach_block_init dbout;

    dbin.bid  = bid;
    dbout.bid  = bid;

    dbin.mode  = stream_obj->mode;
    dbout.mode = DACH_STREAM_WRITABLE;
    if(bid == last_bid) {
      // The last block has a different lenght in almost all cases.
      // so we need to allocate it here
      u64 lastbsize = get_last_block_size(stream_obj->filesize, stream_obj->block_size_actual);
      dbin.length  = lastbsize;
      dbout.length = lastbsize;
      if(lastbsize == 0) {
        dbin.length  = stream_obj->block_size_actual;
        dbout.length = stream_obj->block_size_actual;
        //printf("lstbsize=%llu block_count = %zu\n", lastbsize, block_count);
      }
    }
    else {
      dbin.length  = stream_obj->block_size_actual;
      dbout.length = stream_obj->block_size_actual;
    }
    dbin.data  = stream_obj->mmap_data + pos_pointer;
    dbout.data = NULL;

    stream_obj->dblock_array[bid].obj     = dach_block_new(&dbin);
    stream_obj->dblock_array_out[bid].obj = dach_block_new(&dbout);
    pos_pointer += stream_obj->block_size_actual;

  }
  dblock ddb = stream_obj->dblock_array[0];
  dach_block *db = ddb.obj;
  dach_block_ops *bops = db->ops;
  assert(bops->id(stream_obj->dblock_array[0].obj) == 0);
}

/*
 * Calculate the size of the last block.
 * Warnings, overflow is a real problem.
 */
static size_t get_last_block_size(u64 infilesize, size_t block_size) {
  u32 block_count = get_block_count(infilesize, block_size);
  size_t lbs = infilesize % block_size;
  //assert(lbs < 800);
  return lbs;
}

static void mmap_in_file(dach_stream_obj *stream_obj) {
  stream_obj->mmap_fd  = open(stream_obj->filename, O_RDONLY);
  if (stream_obj->mmap_fd == -1) {
    stream_obj->err_no = errno;
    printf("Fatal: %s\n", strerror(errno));
    dach_perror(DACH_LOG_FATAL, "%s\n", "Error opening input file for reading");
    exit(1);
  }
  assert(stream_obj->filesize > 0);
  //printf("fsize %llu\n", stream_obj->filesize);
  stream_obj->mmap_data = mmap(0, stream_obj->mmap_data_size, PROT_READ, MAP_SHARED, stream_obj->mmap_fd, 0);
  if (stream_obj->mmap_data == MAP_FAILED) {
    close(stream_obj->mmap_fd);
    stream_obj->err_no = errno;
    printf("Error mmap()! %s\n", strerror(errno));
    dach_perror(DACH_LOG_FATAL, "%s", "Error mmapping the file");
    exit(1);
  }
}

/*
static void mmap_out_file(dach_stream_obj *stream_obj) {
  stream_obj->outfile_stream = fopen(stream_obj->filename, "wb+");
  if(stream_obj->outfile_stream == NULL) {
    stream_obj->err_no = errno;
    dach_perror(DACH_LOG_CRITICAL, "Error unable to open output file: %s\n", strerror(errno));
    //return ;//TODO: This might not be the correct error!
  }

  ///\todo tmpfile is 2^31 limited!
  stream_obj->tmpfile           = tmpfile();
  if(stream_obj->tmpfile == NULL) {
    stream_obj->err_no = errno;
    dach_perror(DACH_LOG_CRITICAL, "Error unable to open temp file: %s\n", strerror(errno));
  }
  int fd = fileno(stream_obj->tmpfile);
  if (fd == -1) {
    dach_perror(DACH_LOG_CRITICAL, "%s\n", "Error opening output file for reading");
    stream_obj->err_no = errno; 
  }
  stream_obj->mmap_fd = fd;

  // We need to account for each header being added in a worst case scenario
  // This also assumes we will compress at least header_size for each block. To
  // ensure this happens we will perform a verbatim copy of the data if we
  // cannot compress it at least by header_size bytes!
  stream_obj->out_filesize = stream_obj->filesize + (stream_obj->block_count * stream_obj->hdr_size);


  //printf("block_count  = %u\n", stream_obj->block_count);
  //printf("hdr_size     = %u\n", stream_obj->hdr_size);
  //printf("infile_size  = %lu\n", stream_obj->infile_size);
  //printf("outfile_size = %lu\n", stream_obj->outfile_size);
  lseek (fd, stream_obj->out_filesize - 1, SEEK_SET);
  write (fd, "", 1);
  lseek (fd, 0, SEEK_SET);
  stream_obj->mmap_data = mmap(0, stream_obj->mmap_data_size, PROT_WRITE, MAP_SHARED, fd, 0);
  if (stream_obj->mmap_data == MAP_FAILED) {
    close(fd);
    stream_obj->err_no = errno;
    dach_perror(DACH_LOG_CRITICAL, "%s", "Error mmapping output file for writing");
  }
}*/

void dach_stream_free(dach_stream *stream) {
  DACH_M_VOID_TO_STREAM;
  close(stream_obj->mmap_fd);
  u32 bid = 0;
  for(bid = 0; bid < stream_obj->block_count; bid++) {
    //printf("streamer freeing bid=%u\n", bid);
    dblock db = stream_obj->dblock_array[bid];
    dach_block_free(db.obj);
    db = stream_obj->dblock_array_out[bid];
    dach_block_free(db.obj);
  }
  DACH_FREE(stream_obj->dblock_array);
  DACH_FREE(stream_obj->dblock_array_out);

  int err = munmap(stream_obj->mmap_data, stream_obj->mmap_data_size);
  if(err != 0) {
    fprintf(stderr, "munmap(stream_obj->infile_mmap_data) failed, err_no=%d : %s", errno, strerror(errno));
    assert(0);//TODO: Need dach_status return value.
  }
  DACH_FREE(stream_obj);
  DACH_FREE(stream);
}


/*
 This does not free(ptr) a block it writes it if possible
 */
static void free_block(dach_stream *stream, dach_block *block) {
  DACH_M_VOID_TO_STREAM;
  const u32 bid = block->ops->id(block);
  assert(bid == 0);
  dblock dblk = stream_obj->dblock_array[bid];
  assert(dblk.id == 0);

  if(bid == 0 && !dblk.written_to_stream) {
    write_to_stream(stream_obj, bid);
    if(dblk.written_to_stream > 1) {
      printf("Already written?\n");
    }
    assert(dblk.written_to_stream == 1);
  }
  else if(bid > 0 && block_written(stream_obj, bid - 1)) {
    write_to_stream(stream_obj, bid);
  }
  //fprintf(stderr, "written=%lu write_pos=%lu id=%u\n", p_block->bytes_written, p_block->write_pos, p_block->id);
}

/*
static int block_is_writable(dach_stream_obj *stream_obj, u32 bid) {
  assert(bid < stream_obj->block_count);
  if(bid >= stream_obj->block_count) {
    return 0;
  }
  DACH_BLOCK *block = stream_obj->block_array[bid].priv;
  if(block->written_to_stream) {
    return 0;
  }
  return block->free;
}
*/
static void write_to_stream(dach_stream_obj *stream_obj, const u32 bid) {
  dblock block = stream_obj->dblock_array[bid];
  if(block.written_to_stream == 1) {
    return;
  }
  size_t bytes_written = block.obj->ops->bytes_written(block.obj);
  dach_header hdr = dach_header_new(bytes_written, 1234567890, 1);
  const size_t hdr_size = dach_header_size(); 
  u8 buf[hdr_size]; 
  dach_header_pack(hdr, buf);
  size_t written = fwrite(&buf, 1, hdr_size, stream_obj->outfile_stream);
  dach_header_free(hdr);

  //fprintf(stderr, "blk_written=%lu write_pos=%lu id=%u ", block->bytes_written, block->write_pos, block->id);

  /* \todo NewBlock
  written = fwrite(&stream_obj->outfile_mmap_data[block->start_pos + hdr_size], 1, block->bytes_written, stream_obj->outfile_stream);
   */
  block.written_to_stream++;
  //fprintf(stderr, "written=%lu\n", written);

  /* 
   * The thread that's running here is about to jump boundaries and modify
   * data that another thread might be working on
   */
  //while(we_can_write(bid + 1)) {
  //  write_to_stream;
  //}
  //u32 next_bid = bid + 1;
  //u32 bc =  stream_obj->block_count;

  /*while(next_bid < bc && block_is_writable(stream_obj, next_bid)) {
    write_to_stream(stream_obj, next_bid);
    next_bid++;
  }*/
}

static int block_written(dach_stream_obj *stream_obj, u32 bid) {
  dblock blk     = stream_obj->dblock_array[bid];
  if(blk.written_to_stream > 0) {
    return 1;
  }
  return 0;
}

/*
static size_t calc_block_start(u64 block_length, u32 bid, u32 hdr_size) {
  //dach_block *term = &stream_obj->block_array[bid];
  size_t bstart = 0;
  size_t tlength = block_length + hdr_size;
  bstart = tlength * bid;
  return bstart;
}*/

/* 
 * Exported as a Block Operation
 * @todo: Locking needs to happen here.
 * We should check the stream to make sure that it can handle the amount of threads
 * specified ie the following invariant 
 (max_thread_count <= block_count)
 */

static dach_block *new_block(dach_stream *stream) {
  // This is an OPERATION so it's static and we pass the Opaque pointer
  DACH_M_VOID_TO_STREAM;
  if(stream_obj->blocks_returned == stream_obj->block_count) {
    //printf("b returned=%u b count=%u\n",stream_obj->blocks_returned, stream_obj->block_count);
    //assert(NULL);
    return NULL;
  }
  assert(stream_obj->blocks_returned < stream_obj->block_count);
  dblock block = stream_obj->dblock_array[stream_obj->blocks_returned++];
  return block.obj;
}

static dach_block *new_write_block(dach_stream *stream, dach_block *in_blk) {
  //This is an OPERATION so it's static and we pass the Opaque pointer
  DACH_M_VOID_TO_STREAM;
  size_t in_bid  = in_blk->ops->id(in_blk);
  assert(stream_obj->dblock_array_out[in_bid].obj);
  dach_block *db = stream_obj->dblock_array_out[in_bid].obj;
  assert(db->obj);
  assert(db->ops);
  return db;
  //size_t in_len  = in_blk->op->length(in_blk);
  //size_t in_len  = in_blk->op->length(in_blk);
}



static dach_block_ops *block_ops(dach_stream *stream) {
  DACH_M_VOID_TO_STREAM;
  dblock d = stream_obj->dblock_array[0];
  return d.obj->ops;
}

static size_t blocks(dach_stream *stream) {
  DACH_M_VOID_TO_STREAM;
  //assert((stream_obj->block_count - stream_obj->blocks_returned) < 2);
  return stream_obj->block_count - stream_obj->blocks_returned;
}

/*
static ssize_t block_read(dach_block *block, void *buf, size_t count) {
  assert(NULL != NULL);
  return 1;
}*/

/*
static ssize_t block_write(dblock *block, const void *buf, const size_t count) {
  const char *tmp = buf;
  size_t i = 0;
  for(i = 0; i < count; i++) {
    //fprintf(stderr, "bid=%u wpos=%lu count=%lu\n", p_block->id, p_block->write_pos, count);
    //const char *ch = tmp[i];
    p_block->mmap_out_data[p_block->write_pos] = tmp[i];
    p_block->write_pos++;
    p_block->bytes_written++;
  }
  assert(p_block->write_pos <= p_block->length);
  if(p_block->write_pos > p_block->length) {
    fprintf(stderr, "bid=%u wpos=%lu count=%lu\n", p_block->id, p_block->write_pos, count);
  }
  return 1;
}*/

/*
static u32 block_id(dach_block *block) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  //assert(NULL);
  return p_block->id;
}*/

/*
static size_t block_length(dach_block *block) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  //assert(NULL == NULL);
  /dach_block *ba = p_block->parent_stream->block_array[0];
  if(ba != p_block) {
    assert(1 == 2);
  }///
  //printf("p_block->length == %lu\n", p_block->length);
  //assert(p_block->length < 4194967296);
  return p_block->length;
}*/

/* Exported as an Operation */
/*
static u8 block_getc(dach_block *block) {
  DACH_M_VOID_TO_BLOCK;
  DACH_M_BLOCK_THREAD_ASSERT;
  //assert(NULL != NULL);
  //printf("p_block->read_pos == %lu\n", p_block->read_pos);
  return p_block->mmap_in_data[p_block->read_pos++];
}
*/


static int test_block_size(dach_stream_obj *stream_obj) {
  int rv = 1;
  if(stream_obj->last_block_size == 0) {
    // If stream_obj->last_block_size == 0 it means that
    // the filesize we're encoding must be a multiple
    // of the actual block size or something went wrong.
    assert(stream_obj->filesize % stream_obj->block_size_actual == 0);
    rv = 0;
  }
  return rv;
}

static size_t get_block_count(u64 infilesize, size_t block_size) {
  if(block_size >= infilesize) {
    return 1;
  }; 
  size_t bc = infilesize/block_size;
  bc++;
  if(infilesize % block_size == 0) {
    //printf("Found multiple bsize=%zu bcount = %zu\n", block_size, bc);
    bc--;
  }
  assert(bc > 0);
  return bc;
}

static int err(dach_stream *stream) {
  DACH_M_VOID_TO_STREAM
  return stream_obj->err_no;
}

/*
 dach_stream *dach_stream_new(apr_pool_t *pool, const char *infile, const char *outfile, u32 block_size_hint) {
 assert(block_size_hint > 16);// avoid divide by 0 and daft block sizes
 assert(infile != NULL);
 assert(outfile != NULL);
 dach_stream *stream;
 dach_stream_obj *stream_obj;
 DACH_NEW(stream);
 DACH_NEW(stream_obj);
 stream_obj->err_no      = 0;
 stream_obj->hdr_size    = dach_header_size();
 size_t infsize        = dach_io_file_size(infile);
 //stream_obj->infile      = infile;
 //stream_obj->infile_size       = infsize;
 stream_obj->blocks_returned   = 0;
 stream_obj->block_count       = get_block_count(infsize, block_size_hint);
 stream_obj->block_size_hint   = block_size_hint;
 stream_obj->block_size_actual = block_size_hint;
 stream_obj->last_block_size   = get_last_block_size(infsize, stream_obj->block_size_actual);
 test_block_size(stream_obj);
 stream_obj->outfile_name      = outfile;
 stream_obj->tmpfile           = tmpfile();
 if(stream_obj->tmpfile == NULL) {
 stream_obj->err_no = errno;
 dach_perror(DACH_LOG_CRITICAL, "Error unable to open temp file: %s\n", strerror(errno));
 //return DACH_EACCES;//TODO: This might not be the correct error!
 }
 stream_obj->outfile_stream = fopen(stream_obj->outfile_name, "wb+");
 if(stream_obj->outfile_stream == NULL) {
 stream_obj->err_no = errno;
 dach_perror(DACH_LOG_CRITICAL, "Error unable to open output file: %s\n", strerror(errno));
 //return ;//TODO: This might not be the correct error!
 }
 dach_status d_status;
 mmap_in_file(stream_obj);
 mmap_out_file(stream_obj);
 stream->s_ops = &s_ops;
 stream->priv = stream_obj;
 lazy_create_blocks(stream_obj);
 //set_last_block_size(stream_obj, infsize);
 //CHECK(stream_obj);
 return stream;
 }
 */


/*
 static void internal_piper(pipe) {
 foreach(pipe->operations) {
 foreach(pipe->block_pipes[]) {
 operation->get_block_in(dach_operation);
 operation->get_block_out(dach_operation);
 }
 }
 }
 */


/*
 dach_stream *dach_stream_open(apr_pool_t *pool, apr_file_t *infile_fd) {
 assert(infile_fd != NULL);
 dach_stream *stream;
 dach_stream_obj *stream_obj;

 stream = apr_palloc_debug(pool, sizeof(*stream), APR_POOL__FILE_LINE__);
 stream_obj = apr_palloc_debug(pool, sizeof(*stream_obj), APR_POOL__FILE_LINE__);
 //H_NEW(stream);
 //H_NEW(stream_obj);
 stream_obj->err_no = 0;
 stream_obj->hdr_size = dach_header_size();
 stream_obj->blocks_returned   = 0;
 stream->s_ops = &s_ops;
 stream->priv = stream_obj;
 return stream;
 }*/

/*

 dach_pipe *dach_pipe_new_old(dach_stream *ds1, size_t argcount, ...) {
 assert(argcount >= 1);
 dach_pipe_impl      *pip_impl;
 dach_stream_impl    *source = (dach_stream_impl *)ds1;

 DACH_NEW(pip_impl);
 DACH_NEW(pip_impl->obj);
 DACH_NEW(pip_impl->ops);
 DACH_N_NEW(16,pip_impl->obj->oper_array);

 pip_impl->ops->wait       = dach_pipe_wait;
 pip_impl->obj->source     = source;
 pip_impl->obj->oper_count = argcount;
 dach_operation_impl *dop_impl;

 va_list ap;
 va_start(ap, argcount);
 size_t i = 0;
 for(i = 0; i < argcount; i++) {
 dop_impl                      = va_arg(ap, dach_operation_impl*);
 dop_impl->obj->type           = DACH_TYPE_OPERATION;
 dop_impl->obj->mode           = DACH_STREAM_READWRITE;
 pip_impl->obj->oper_array[i]  = *dop_impl;
 //printf("%p\n", dop_impl->obj->out_blk);
 }
 va_end(ap);
 return (dach_pipe*)pip_impl;
 }
*/

#undef DACH_M_VOID_TO_STREAM
#undef DACH_M_VOID_TO_BLOCK

