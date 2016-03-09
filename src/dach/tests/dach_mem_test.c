#include "dach/dach_block.h"
#include "dach/dach_stream.h"
#include "cool_utils.h"
#include <assert.h>
#include <unistd.h>

int main() {
  dach_block_init db;
  DACH_CONFIG     dc;

  dc.mode           = "r";
  dc.threads        = 1;
  //DACH_TYPE        type = DACH_TYPE_STREAM;
  u32 rcn   = rand_cycle_number(RAND_TEST_CYCLE);

  size_t filesize  = new_random_size(DACH_TEST_MAX_FILE_SIZE);
  char  filename[] = "/tmp/dach.test.data.XXXXXX";
  int   fd         = mkstemp(filename);
  assert(write(fd, buff, filesize) == (int)filesize);
  close(fd);
  dc.file          = filename;

  size_t x = 0;
  const size_t max_runs = 1000;
  for(x = 0; x < max_runs; x++) {
    db.length =  new_random_size(DACH_TEST_MAX_FILE_SIZE);
    db.mode   = DACH_STREAM_WRITABLE;
    db.bid    = 0;
    db.data   = NULL;
    dach_block *blk = dach_block_new(&db);
    dach_stream *ds = dach_stream_new(&dc);

    dach_stream_free(ds);
    dach_block_free(blk);
  }
  unlink(filename);
  printf("ok ... dach_mem_test\n");
  return 0;
}

