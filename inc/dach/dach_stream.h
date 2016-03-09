#ifndef DACH_STREAM_H
#define DACH_STREAM_H
//#include "dach_apr.h"
#include "dach_block.h"
#include "dach_config.h"
#include "dach_types.h"

/* 
 NOTE: Blocks are really an implementation detail.
 Can we perform things like filtering, tokens, line 
 by line ready etc without exposing blocks?
 */

typedef struct dach_operation  dach_operation;
typedef struct dach_pipe       dach_pipe;
typedef struct dach_stream     dach_stream;

/*
 Map Type might be a better name for this.
 */
typedef enum DACH_BLOCK_BOUNDARY {
  DACH_BYTE       = 0,
  DACH_ASCII_CHAR,    //tbd
  DACH_WIDE_CHAR,     //tbd
  DACH_BOUNDARY_FUNC, //tbd
} DACH_BLOCK_BOUNDARY;


typedef struct config {
  const char *file;
  const char *mode;       // "r" || "w"
  size_t     block_size; // Hint only. 0 if no blocking is to take place.
  union {
    const char  *file;
    dach_stream *ds;
  } source;
  int source_type;
  int sink_type;
  union {
    const char  *file;
    dach_stream *ds;
  } sink;
  int        threads;    // > 0
  int        pad;
} DACH_CONFIG;

typedef struct dach_pipe_ops {
  ssize_t (*add       )(dach_pipe *pip, dach_operation *oper);
  ssize_t (*start     )(dach_pipe *pip);
  ssize_t (*tofile    )(dach_pipe *pip, FILE *fd);
  ssize_t (*wait      )(dach_pipe *pip);
} dach_pipe_ops;

struct dach_pipe {
  void          *obj;
  dach_pipe_ops *ops;
};

typedef struct dach_stream_ops {
  dach_block_ops * (*block_ops  )(dach_stream *stream);
  size_t           (*blocks     )(dach_stream *stream);
  dach_block     * (*new_block  )(dach_stream *stream);
  dach_block     * (*write_block)(dach_stream *stream, dach_block *block);
  void             (*free_block )(dach_stream *stream, dach_block *block);
  int              (*err        )(dach_stream *stream);
} dach_stream_ops;

struct dach_stream {
  void            *obj;
  dach_stream_ops *ops;
};

typedef void (*dach_operation_callback)(dach_operation *oper);

typedef struct dach_operation_ops {
  ssize_t (*read        )(dach_operation *oper,       void *buf, size_t count);
  ssize_t (*write       )(dach_operation *oper, const void *buf, size_t count);
  void    (*operation   )(dach_operation *oper);// User defined function
} dach_operation_ops;

struct dach_operation {
  void               *obj;
  dach_operation_ops *ops;
};


dach_operation *dach_operation_new(dach_operation_callback callback);
void            dach_operation_free(dach_operation *ptr);

/* 
 * Ideally the pipe function should be variadic so we can take
 * advantage of the whole chain of operations running concurrently
 * ie if we do two at a time we have a synchronization point 
 * waiting for each pipe functon to finish
 */
dach_pipe *dach_pipe_new(dach_stream *dsr, dach_operation *dop);
void       dach_pipe_free(dach_pipe *pip);

dach_stream *dach_stream_new(DACH_CONFIG *dc);
void         dach_stream_free(dach_stream *stream);
void         dach_stream_close(dach_stream *stream);


#endif /* dach_STREAM_H */
