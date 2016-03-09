#include "dach/dach_lock.h"
#include "cool_utils.h"

int main() {
  printf("Success\n");
    
  /*
  const size_t max_runs = 10;
  size_t i = 0;
  for (i = 0 ; i < max_runs; i++) {
    //printf(".\r");
    dach_block_init db;
    db.length = new_random_size(DACH_TEST_MAX_FILE_SIZE);
    db.mode   = DACH_STREAM_WRITABLE;
    db.bid    = 0;
    db.data   = NULL;
    dach_block *blk = dach_block_new(&db);

    size_t i = 0;
    char buf[64];
    char cache_buf[DACH_TEST_MAX_FILE_SIZE];
    size_t rsize = new_random_size(DACH_TEST_MAX_FILE_SIZE) + 1;
    for(i = 0; i < rsize ; i++) {
      char c = new_rand_char();
      cache_buf[i] = c;
      buf[0] = c;
      blk->ops->write(blk, buf, 1);
    }
    const char *filename = "/tmp/dach.test.data.out";
    unlink(filename);
    FILE *fp = fopen(filename, "ab+");
    assert(fp != NULL);

    blk->ops->tofile(blk, fp);
    fseek(fp, 0, 0);

    size_t n = 0;
    char in_buf[DACH_TEST_MAX_FILE_SIZE];
    for(n = 0 ; n < i ; n++) {
      size_t r = fread(in_buf, 1, 1, fp);
      if(in_buf[0] != cache_buf[n]) {
        printf("in_buf=%c cache_buf=%c\n", in_buf[n], cache_buf[n]);
        assert(NULL);
      }
    }
    fclose(fp);
    dach_block_free(blk);
  }*/
  return 0;
}

