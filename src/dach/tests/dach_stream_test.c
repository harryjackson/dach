#include "dach/dach_stream.h"
#include "dach/dach_copy.h"
#include "dach/dach_minunit.h"
#include "dach/dach_mem.h"
#include "cool_utils.h"
#include <assert.h>
#include <apr-1/apr_pools.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

static char buff[DACH_TEST_MAX_FILE_SIZE];

static size_t test_file_stream_reader(const size_t bsize, const size_t filesize);
static size_t test_file_stream_writer(const size_t bsize, const size_t filesize);

static int test_run[4096];

static void   stream_test_copy(dach_operation *op);
static void   new_random_block(void);
static int    compare_files(FILE *fp1, FILE *fp2);


static int compare_files(FILE *fp1, FILE *fp2) {
  char c1 = getc(fp1);
  char c2 = getc(fp2);
  assert(c1 != EOF);
  assert(c2 != EOF);
  while ((c1 != EOF) && (c2 != EOF)) {
    //printf("%c == %c\n", c1, c2);
    assert(c1 == c2);
    c1 = getc(fp1);
    c2 = getc(fp2);
  }
  return 0;
}

static const int buf_size = 4096;
static u8        stream_test_copy_buf[4096] = {0};
static void stream_test_copy(dach_operation *op) {
  ssize_t count = 0;
  while((count = op->ops->read(op, &stream_test_copy_buf, buf_size)) >= 0) {
    //printf("writing\n");
    op->ops->write(op, stream_test_copy_buf, (size_t)count);
  }
}

void new_random_block(void) {
  size_t i = 0;
  for(i = 0; i < DACH_TEST_MAX_FILE_SIZE; i++) {
    char c = new_rand_char();
    buff[i] = c;
  }
}

//size_t new_random_size(size_t s) {
//  assert(s > 0);
//  return (rand_size_t() % s) + 1;// Always > 0;
//}

size_t test_file_stream_reader(size_t bsize, size_t filesize) {
  assert(filesize <= DACH_TEST_MAX_FILE_SIZE);
  assert(bsize > 0);
  assert(filesize > 0);

  char filename[] = "/tmp/dach.test.data.XXXXXX";
  int fd = mkstemp(filename);
  //char *filename = template;
  assert(fd > 0);
  assert(write (fd, buff, filesize) >= 0);
  close(fd);

  DACH_CONFIG read_conf;
  read_conf.file = filename;
  read_conf.mode    = "r";
  read_conf.threads = 4;
  read_conf.block_size = bsize;
  dach_stream *dsr = dach_stream_new(&read_conf);
  dach_block *db;

  u8 last;
  size_t bid = 0;
  size_t bcount = dsr->ops->blocks(dsr);
  for(bid = 0; bid < bcount; bid++) {
    db = dsr->ops->new_block(dsr);
    bsize = db->ops->length(db);
    size_t pos = 0;
    for(pos = 0; pos < bsize; pos++) {
      u8 c = db->ops->get_c(db);
      last = c;
    }
  }
  unlink(filename);
  dach_stream_free(dsr);
  return 0;
}

size_t test_file_stream_writer(const size_t bsize, const size_t filesize) {
  assert(filesize <= DACH_TEST_MAX_FILE_SIZE);
  assert(bsize > 0);
  assert(filesize > 0);
  printf("bsize = %zu filesize=%zu\n", bsize, filesize);

  char  filename[] = "/tmp/dach.test.data.XXXXXX";
  int   fd         = mkstemp(filename);
  assert(write(fd, buff, filesize) == (int)filesize);
  //unlink(filename);
  DACH_CONFIG read_conf;
  read_conf.file       = filename;
  read_conf.mode       = "r";
  read_conf.threads    = 4;
  read_conf.block_size = bsize;
  dach_stream    *dsr  = dach_stream_new(&read_conf);
  dach_operation *dop  = dach_operation_new(&stream_test_copy);
  dach_operation *dop2 = dach_operation_new(&stream_test_copy);
  assert(dop->ops);
  assert(dop->obj);
  assert(&stream_test_copy == dop->ops->operation);
  dach_pipe *pip           =  dach_pipe_new(dsr, dop);
  pip->ops->add(pip, dop2);
  pip->ops->start(pip);

  const char *out_filename = "/tmp/dach.test.data.out";
  unlink(out_filename);
  FILE *fp = fopen(out_filename, "ab+");
  assert(fp != NULL);

  pip->ops->tofile(pip, fp);
  FILE *file_in     = fdopen(fd, "wb+");

  fseek(fp, 0, 0);
  fseek(file_in, 0, 0);
  compare_files(file_in, fp);

  fclose(file_in);
  fclose(fp);
  unlink(filename);
  dach_operation_free(dop);
  dach_stream_free(dsr);
  dach_pipe_free(pip);
  return 0;
}


int main() {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    fprintf(stdout, "Current working dir: %s\n", cwd);
  } else {
    perror("getcwd() error");
  };

  /*
  char  r_filename[] = "/tmp/dach.test.data.XXXXXX";
  int   r_fd         = mkstemp(r_filename);
  FILE *r_file_d     = fdopen(r_fd, "wb+");

  char  w_filename[] = "/tmp/dach.test.data.XXXXXX";
  int   w_fd         = mkstemp(w_filename);
  FILE *w_file_d     = fdopen(w_fd, "wb+");

  size_t filesize = 652;
  assert(w_fd > 2);
  assert(fwrite(buff, 1, filesize, w_file_d) == filesize);
  fclose(w_file_d);
  */
  new_random_block();
  /*
   * Regressions.
   * The following numbers failed at one point.
   */
  test_file_stream_writer(549, 652);

  /*
   * Edges
   *  case: Block Size Multiple of Fileize
   *  case: Block Size > Fileize
   *  case: Block Size == Fileize
   */
  test_file_stream_reader(100, 200);
  test_file_stream_reader(200, 100);
  test_file_stream_reader(200, 200);

  test_file_stream_writer(100, 200);
  test_file_stream_writer(200, 100);
  test_file_stream_writer(200, 200);

  size_t x = 0;
  const size_t max_runs = 10;
  for(x = 0; x < max_runs; x++) {
    u32 rcn = rand_cycle_number(RAND_TEST_CYCLE);
    test_run[rcn]++;
    printf("REPEAT_CYLCE_NUMBER==%u\n", rcn);
    srand(rcn);
    size_t i = 0;//test count
    size_t fcount = (rand_size_t() % 10) + 1;
    for(i = 0; i < fcount; i++) {
      size_t filesize = new_random_size(DACH_TEST_MAX_FILE_SIZE);
      size_t bsize    = new_random_size(DACH_TEST_MAX_FILE_SIZE);
      test_file_stream_reader(bsize, filesize);
    }
    for(i = 0; i < fcount; i++) {
      size_t filesize = new_random_size(DACH_TEST_MAX_FILE_SIZE);
      size_t bsize    = new_random_size(DACH_TEST_MAX_FILE_SIZE);
      test_file_stream_writer(bsize, filesize);
    }
  }

  size_t i = 0;
  for(i = 0; i < max_runs; i++) {
    if(test_run[i] > 0) {
      printf("rcn=%zu run_count=%d\n", i, test_run[i]);
    }
  }
  return 0;
}

