#include "dach/dach_io.h"
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static off_t dach_get_file_size(const char *filename);

off_t dach_io_file_size(const char *fname)
{  
  assert(fname != NULL);
  assert(dach_io_file_exists(fname));
  return dach_get_file_size(fname);
}

int dach_io_file_exists(const char *filename)
{
  struct stat buffer;
  //printf("%s\n", filename);
  int status = stat (filename, &buffer);
  if(status == -1) {
    fprintf(stderr, "%s", strerror(errno));
  }
  return (stat (filename, &buffer) == 0);
}

static off_t dach_get_file_size(const char *filename)
{
  struct stat stbuf;
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    close(fd);
    return -1;
  }
  if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
    close(fd);
    return -1;
  }
  close(fd);
  //printf("%s == %lld\n", filename, stbuf.st_size);
  return stbuf.st_size;
}


int dach_open_file(apr_pool_t *pool, apr_file_t **file, const char* fname, unsigned short OPTIONS)
{
  apr_status_t rv = apr_file_open(
      file, // new file handle
      fname, // file name
      OPTIONS |
      0, // flags
      APR_OS_DEFAULT |
      0, // permissions
      pool // memory pool to use
      );
  return rv;
}

/*
static void dach_log(const char* msg, const int level, const char* sourcefilename, const int line);
static int dach_slurp_file_into_ram(unsigned char * slab, const size_t fsize, const char* fname);
static int dach_count_chars(const size_t jump, int* int_block, const size_t bsize, const char* filename);
static unsigned short int dach_get_apr_write_opts(void);
static unsigned short int dach_get_apr_read_opts(void);
static int dach_slurp(apr_file_t *file, struct dach_slab *into, apr_ssize_t bytes);
*/



/*
static unsigned short int dach_get_apr_write_opts(void) {
  return APR_WRITE         |
         APR_CREATE        |
         APR_FOPEN_BINARY  | // binary mode (ignored on UNIX)
         APR_FOPEN_XTHREAD | // allow multiple threads to use file
         0;
}  

static unsigned short int dach_get_apr_read_opts(void)
{
  return APR_READ          |
         APR_FOPEN_BINARY  | // binary mode (ignored on UNIX)
         APR_FOPEN_XTHREAD | // allow multiple threads to use file
         0;
} 

static void dach_log(const char* msg, const int level, const char* sourcefilename, const int line) {
  char buf[32];
  char *out = malloc(strlen(msg) + strlen(sourcefilename)+128);
  sprintf(buf, "line = %d", line);
  strcpy(out, msg);
  strcpy(out, ": ");
  strcpy(out, sourcefilename);
  strcpy(out, buf);
  fprintf(stderr, "%s\n", out);
  free(out);
}


static int dach_slurp_file_into_ram(unsigned char * slab, const size_t fsize, const char* fname) {
  const size_t testsize = dach_get_file_size(fname);
  if(! ( testsize > 0) ) {
    dach_log("Error reading file size from dach_get_file_size(fname)", 0, __FILE__, __LINE__);
  }
  //printf("slap=%lu == fsize=%lu\n", testsize, fsize);
  assert(testsize == fsize);
  FILE *f = fopen(fname, "rb");
  if(f != NULL) {
    size_t read = fread(slab, sizeof(char),fsize, f);
    //printf("read=%lu == fsize=%lu\n", read, fsize);
    assert(read == (size_t)fsize);
    fclose(f);
    slab[fsize] = '\0';
    assert(strlen((const char *)slab) == (fsize));
    return 0;
  }
  //fprintf(stderr, "Error slurp_file_into_ram(slab, fname) %d!", __LINE__);
  return -1;
}
*/

/*
static int dach_count_chars(const size_t jump, int* int_block, const size_t bsize, const char* filename) {
  FILE *fp;
  const int dach_BUFF_SIZE = 4065536;// Why?
  int cbufsize  = sizeof(char) * dach_BUFF_SIZE;
  unsigned char *buffer  = malloc(cbufsize);
  if (buffer == NULL || int_block == NULL) {
    return 1;
  }
  fp = fopen(filename, "r+"); 
  if(fp == NULL) {
      return 1;
  }
  int canread = 1;
  while(canread) {
    size_t read = fread(buffer, sizeof(char), cbufsize, fp);
    if (ferror(fp)) {
      return 1;
    }
    size_t i = 0;
    for(i = 0; i < read ; i += jump ) {
      int dst = 0;
      memcpy(&dst, &buffer[i], jump);
      int_block[dst]++;
    }
    if (feof(fp)) {
      canread = 0;
    }
  }
  fclose(fp);
  free(buffer);
  return 0;
}
*/

/*
 

static int dach_slurp(apr_file_t *file, struct dach_slab *into, apr_ssize_t bytes)
{
  apr_size_t read;
  apr_status_t rv = apr_file_read_full(file, into->slab, bytes, &read);
  return rv;
}
*/
