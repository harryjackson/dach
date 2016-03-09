#include "dach/dach.h"
#include "dach/dach_apr.h"
#include "dach/dach_ar64.h"
#include "dach/dach_error.h"
#include "dach/dach_io.h"
#include "dach/dach_header.h"
#include "dach/dach_log.h"
#include "dach/dach_ppm0.h"
#include "dach/dach_stream.h"
#include "dach/dach_types.h"
#include <assert.h>
#include <apr-1/apr_queue.h>
#include <apr-1/apr_thread_mutex.h>
#include <apr-1/apr_thread_pool.h>
#include <apr-1/apr_strings.h>
#include <apr-1/apr_mmap.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


void dach_sleep(int sec, int usec) {
  struct timeval tv;
  tv.tv_sec = sec;
  tv.tv_usec = usec;
  select(0, NULL, NULL, NULL, &tv);
}

static const char    *const_file_ext    = ".dch";
static const size_t  const_file_ext_len = 4;

#define DACH_M_MAX_STREAM_BUFFER 4096
static const u32 max_buf = 4096;

static int copy_file(DACH_CONFIG *dc);
static int encode(DACH_CONFIG *dc);
static int decode(DACH_CONFIG *dc);
static int decode_stream(DACH_CONFIG *dc);
static int encode_stream(DACH_CONFIG *dc);
static int info(DACH_CONFIG *dc);

int dach_app_initialize(int *argc, char const *const **argv, char const *const **env)
{
	return apr_app_initialize(argc, argv, env);
}

int dach_initialize(void)
{
	apr_status_t rv = apr_initialize();
	return rv;
}

void dach_terminate(void)
{
	apr_terminate();
}

int dach_copy_file(DACH_CONFIG *dc)
{
  return copy_file(dc);
}

int dach_info_file(DACH_CONFIG *dc)
{
	return info(dc);
}

int dach_estream_file(DACH_CONFIG *dc)
{
  return encode_stream(dc);
}

int dach_dstream_file(DACH_CONFIG *dc)
{
  return decode_stream(dc);
}

static void *ap_thread_decode_stream(apr_thread_t *apt, void *v_stream)
{
  dach_stream *stream = (dach_stream *) v_stream;
  dach_stream_ops *s_ops = stream->ops;
  dach_block_ops  *b_ops = s_ops->block_ops(stream);

  while (s_ops->blocks(stream)) {
    dach_block *blk = s_ops->new_block(stream);
    if (blk != NULL) {
      dach_ppm_model *mod = dach_ppm0_new_();
      ARC *arc = dach_new_arcoder_(mod, blk);
      u32 pos = 0;
      const u32 bsize = b_ops->length(blk);
      //printf("bsize = %u\n", bsize);
      //exit(0);
      while (pos < bsize) {
        u8 c = b_ops->get_c(blk);
        //fprintf(stdout, "%c", c);
        dach_decode(arc);
        //dach_encode(arc, c);
        pos++;
      }
      dach_stop_encoder(arc);
      dach_delete_arcoder(arc);
      dach_ppm0_free_(mod);
      //fprintf(stderr, "finishing block=%u\n", b_ops->id(blk));
      s_ops->free_block(stream, blk);
    }
  }
  return v_stream;
}


static int copy_file(DACH_CONFIG *dc) {
  assert(NULL);
  return 0;
}


static void *ap_thread_encode_stream(apr_thread_t *apt, void *v_stream)
{
  dach_stream *stream = (dach_stream *) v_stream;
  dach_stream_ops *s_ops = stream->ops;
  dach_block_ops *b_ops = s_ops->block_ops(stream);
  printf("...\n");
  while (s_ops->blocks(stream)) {
    dach_block *blk = s_ops->new_block(stream);
    if (blk != NULL) {
      dach_ppm_model *mod = dach_ppm0_new_();
      ARC *arc = dach_new_arcoder_(mod, blk);
      u32 pos = 0;
      const u32 bsize = b_ops->length(blk);
      //printf("bsize = %u\n", bsize);
      //exit(0);
      while (pos < bsize) {
        u8 c = b_ops->get_c(blk);
        //fprintf(stdout, "%c", c);
        dach_encode(arc, c);
        pos++;
      }
      dach_stop_encoder(arc);
      dach_delete_arcoder(arc);
      dach_ppm0_free_(mod);
      //fprintf(stderr, "finishing block=%u\n", b_ops->id(blk));
      s_ops->free_block(stream, blk);
    }
  }
  return v_stream;
}

static void *ap_non_thread_encode_stream(void *v_stream)
{
  dach_stream *stream = (dach_stream *) v_stream;
  dach_stream_ops *s_ops = stream->ops;
  dach_block_ops *b_ops = s_ops->block_ops(stream);
  int loopy = 0;
  while (s_ops->blocks(stream)) {
    loopy++;
    dach_block *blk = s_ops->new_block(stream);
    if (blk != NULL) {
      dach_ppm_model *mod = dach_ppm0_new_();
      ARC *arc = dach_new_arcoder_(mod, blk);
      u32 pos = 0;
      const u32 bsize = b_ops->length(blk);
      //printf("bsize = %u\n", bsize);
      //exit(0);
      int inner_loop = 0;
      while (pos < bsize) {
        inner_loop++;
        u8 c = b_ops->get_c(blk);
        //fprintf(stdout, "encoding char: %c\n", c);
        dach_encode(arc, c);
        pos++;
      }
      assert(loopy == 1);
      dach_stop_encoder(arc);
      printf("inner_loop = %d\n", inner_loop);

      dach_ppm0_free_(mod);
      dach_delete_arcoder(arc);
      //fprintf(stderr, "finishing block=%u\n", b_ops->id(blk));
      //s_ops->free_block(stream, blk);
    }
  }
  return v_stream;
}

static int decode_stream(DACH_CONFIG *dc) {
  const char *infile = dc->file;
  assert(infile != NULL);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  const char *ap_infile = apr_pstrdup(pool, infile);
  const char *ap_outfile = apr_pstrcat(pool, ap_infile, const_file_ext, NULL);
  printf("outfile is: %s\n", ap_outfile);
  size_t filesize = dach_io_file_size(ap_infile);
  if(filesize == 0) {
    assert(0);
  }
  dach_stream *stream = NULL;// dach_stream_new(pool, ap_infile, ap_outfile, dc->block_size);
  apr_queue_t *queue = NULL;

  int stream_thread_limit = stream->ops->blocks(stream);
  printf("stream_thread_limit==%d\n", stream_thread_limit);

  if(dc->threads > stream_thread_limit) {
    dc->threads = stream_thread_limit;
    assert(NULL);
  }
  int num_threads  = dc->threads;
  int init_threads = num_threads;
  int max_threads  = num_threads;
  int rv = apr_queue_create(&queue, max_threads, pool);
  apr_thread_pool_t *apt_pool = NULL;
  rv = apr_thread_pool_create(&apt_pool, init_threads, max_threads, pool);
  if (rv != APR_SUCCESS) {
    //apr_thread_create(&thread[t], NULL, ap_thread_stream, (void *)stream, pool) != APR_SUCCESS) {
    printf("ERROR; return code from apr_thread_pool_create(...) is %d\n",rv);
    exit(-1);
  }
  int owner = 1;
  apr_thread_t *threads[num_threads];
  void *status[num_threads];
  int rc;
  long t;
  for (t = 0; t < num_threads; t++) {
    printf("creating thread %ld\n", t);
    rv = apr_thread_pool_push(apt_pool, ap_thread_decode_stream, (void *)stream, 1, &owner);
    if (rv != APR_SUCCESS) {
      printf ("ERROR; return code from apr_thread_pool_push(...) is %d\n", rv);
      exit(-1);
    }
  }
  dach_sleep(0, 1000);
  printf("canceling threads\n");
  printf("Busy Count %lu\n", apr_thread_pool_busy_count(apt_pool));
  rv = apr_thread_pool_tasks_cancel(apt_pool, &owner);
  if (rv != APR_SUCCESS) {
    printf ("ERROR; return code from pthread_create() is %d\n", rv);
    exit(-1);
  }
  rv = apr_thread_pool_destroy(apt_pool);
  if (rv != APR_SUCCESS) {
    printf ("ERROR; return code from pthread_create() is %d\n", rv);
    exit(-1);
  }
  dach_stream_free(stream);
  apr_pool_destroy(pool);
  return 0;
}


static int encode_stream(DACH_CONFIG *dc) {
  const char *infile = dc->file;
	assert(infile != NULL);
	apr_pool_t *pool = NULL;
	apr_pool_create(&pool, NULL);
	const char *ap_infile = apr_pstrdup(pool, infile);
	const char *ap_outfile = apr_pstrcat(pool, ap_infile, const_file_ext, NULL);
	//printf("outfile is: %s\n", ap_outfile);
	size_t filesize = dach_io_file_size(ap_infile);
  if(filesize == 0) {
    assert(0);
  }
  dach_stream *stream = NULL;//dach_stream_new(pool, ap_infile, ap_outfile, dc->block_size);
	apr_queue_t *queue = NULL;
  int stream_thread_limit = stream->ops->blocks(stream);
  //printf("stream_thread_limit==%d\n", stream_thread_limit);
  if(dc->threads > stream_thread_limit) {
    dc->threads = stream_thread_limit;
    //assert(NULL);
  }
  int num_threads  = dc->threads;
  int init_threads = num_threads;
  int max_threads  = num_threads;
	int rv = apr_queue_create(&queue, max_threads, pool);
	apr_thread_pool_t *apt_pool = NULL;
	rv = apr_thread_pool_create(&apt_pool, init_threads, max_threads, pool);
	if (rv != APR_SUCCESS) {
		//apr_thread_create(&thread[t], NULL, ap_thread_stream, (void *)stream, pool) != APR_SUCCESS) {
		printf("ERROR; return code from apr_thread_pool_create(...) is %d\n",rv);
		exit(-1);
	}
	int owner = 1;
	apr_thread_t *threads[num_threads];
	void *status[num_threads];
	int rc;
	long t;
  printf("here?\n");
  ap_non_thread_encode_stream(stream);
  /*
	for (t = 0; t < num_threads; t++) {
    printf("ct\n");
		//printf("creating thread %ld\n", t);
		rv = apr_thread_pool_push(apt_pool, ap_thread_encode_stream, (void *)stream, 1, &owner);
    printf("|||\n");
		if (rv != APR_SUCCESS) {
			printf ("ERROR; return code from apr_thread_pool_push(...) is %d\n", rv);
			exit(-1);
		}
  }
   */
  //select(NULL,NULL,NULL,NULL,1000);//Hack to sleep for milliseconds
  /*while(apr_thread_pool_busy_count(apt_pool)) {
    //select(NULL,NULL,NULL,NULL,10);//Hack to sleep for milliseconds
    //printf("Busy Count %lu\n", apr_thread_pool_busy_count(apt_pool));
  }*/
  //printf("canceling threads\n");
  rv = apr_thread_pool_tasks_cancel(apt_pool, &owner);
	if (rv != APR_SUCCESS) {
		printf ("ERROR; return code from pthread_create() is %d\n", rv);
		exit(-1);
	}
	rv = apr_thread_pool_destroy(apt_pool);
	if (rv != APR_SUCCESS) {
		printf ("ERROR; return code from pthread_create() is %d\n", rv);
		exit(-1);
	}
	dach_stream_free(stream);
	apr_pool_destroy(pool);
	return 0;
}
int dach_encode_file(DACH_CONFIG *dc)
{
	return encode(dc);
}

int dach_decode_file(DACH_CONFIG *dc)
{
	return decode(dc);
}


static int encode(DACH_CONFIG *dc)
{
	char *error;
  const char *fname = dc->file;
	size_t fsize = dach_io_file_size(fname);
	int size = strlen(fname) + strlen(const_file_ext);
	char *outfilename = malloc(sizeof(char) * size + 1);
	strcat(outfilename, fname);
	strcat(outfilename, const_file_ext);
	fprintf(stderr,	"size = %i outfilename=%s newsize = %lu \n", size, outfilename, strlen(outfilename));
	FILE *fd_out = fopen(outfilename, "w");
	if (!fd_out) {
		fprintf(stderr, "Fatal: Unable to open file for writing:%s\n", outfilename);
		exit(1);
	}
  free(outfilename);
	size_t pos = 0;
	int fd_mmap = open(fname, O_RDONLY);
	if (fd_mmap == -1) {
		perror("Error opening file for reading, exiting");
		exit(EXIT_FAILURE);
	}
	char *data = mmap(0, fsize, PROT_READ, MAP_SHARED, fd_mmap, 0);
	if (data == MAP_FAILED) {
		close(fd_mmap);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
	}
	if (fsize > 0) {
		dach_ppm_model *mod = dach_ppm0_new_();
		ARC *arc = dach_new_arcoder(mod, stdin, fd_out, 256, (u64) fsize);
		while (pos < fsize) {
			unsigned char c = data[pos];
			dach_encode(arc, c);
			pos++;
		}
		dach_stop_encoder(arc);
		dach_delete_arcoder(arc);
		dach_ppm0_free_(mod);
	}
	close(fd_mmap);
	munmap(data, fsize);
	fclose(fd_out);

	return 0;
}

static int decode(DACH_CONFIG *dc)
{
  const char *fname = dc->file;
	FILE *fdec;
	char *mode = "rb";
	fdec = fopen(fname, mode);
	if (fdec == NULL) {
		fprintf(stderr,
			"Can't open input file: %s!\n",
			fname);
		exit(1);
	}
	char *ext = strrchr(fname, '.');
	if (ext == NULL || strcmp(ext, const_file_ext) != 0) {
		fprintf(stderr,
			"Missing file extension on file: %s %s!\n",
			ext, fname);
		exit(1);
	}
	ext[1] = '1';
	ext[2] = '\0';
	int size = strlen(fname);
	char *outfilename = malloc(sizeof(char) * size + 1);
	strcat(outfilename, fname);
	ext[0] = '.';
	fprintf(stderr,	"size = %i outfilename=%s newsize = %lu \n", size, outfilename, strlen(outfilename));
	FILE *fd_out = fopen(outfilename, "w");
	if (!fd_out) {
		fprintf(stderr,"Fatal: Unable to open file for writing:%s\n", outfilename);
		exit(1);
	}
  free(outfilename);
	dach_ppm_model *mod = dach_ppm0_new_();
	ARC *arc = dach_new_arcoder(mod, fdec, stdout, 256, 0);
	dach_start_decoder(arc);
	int i = 0;
	u8 buf[DACH_M_MAX_STREAM_BUFFER] = { 0 };
	u32 p_buf = 0;

	while (!dach_eof(arc)) {
		buf[p_buf++] = dach_decode(arc);
		//fprintf(stderr, "sym = %c\n", sym);
		if (p_buf == max_buf) {
			fwrite(buf, 1, p_buf, fd_out);
			p_buf = 0;
		}
	}
	fwrite(buf, 1, p_buf, fd_out);
	fflush(fd_out);
	fclose(fd_out);
  dach_delete_arcoder(arc);
	dach_ppm0_free_(mod);
	return 0;
}

static int info(DACH_CONFIG *dc) {
  const char *infile = dc->file;
  assert(infile != NULL);
  const size_t max_pname = 1024;
  if (strlen(infile) >= max_pname) {
    dach_perror(DACH_LOG_CRITICAL, "Input path is >= %lu", max_pname);
    return DACH_EINVAL;
  }
  char *p;
  p = malloc(strlen(infile) + 1);
  char *nl = strcpy(p, infile);
  //printf("%s\n", nl);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  //apr_file_t * fd = NULL;
  const char *ap_infile = apr_pstrdup(pool, infile);
  unsigned short flags = APR_READ
  | APR_FOPEN_BINARY //binary mode(ignored on UNIX)
  | APR_FOPEN_XTHREAD	//allow multiple threads to use file
  |  0;

  apr_file_t *apr_infile_fd;
  int rv = dach_open_file(pool, &apr_infile_fd, ap_infile, flags);
  if (rv != APR_SUCCESS) {
    printf("ERROR; unable to open file %s\n", ap_infile);
    exit(-1);
  }
  size_t len = strlen(ap_infile);
  if (len < const_file_ext_len) {
    return DACH_EINVAL;
  }
  char *ext = strrchr(ap_infile, '.');
  if (!ext) {
    return DACH_EINVAL;
  }
  if (strncmp(ext, const_file_ext, 4) != 0) {
    return DACH_EINVAL;
  }
  //fprintf(stderr, "ext == %s\n", ext);

  size_t hd_size = sizeof(char) * dach_header_size();
  //fprintf(stderr, "hd_size == %zu\n", hd_size);
  u8 *buf = malloc(hd_size);
  if(buf == NULL) {
    return DACH_ENOMEM;
  }
  dach_header hd1 = dach_header_new_blank();
  if(hd1 == NULL) {
    return DACH_ENOMEM;
  }
  size_t read_size  = hd_size;
  apr_off_t seek_where = 0;
  size_t bid        = 0;
  size_t block_size_tot = 0;
  size_t hdr_size_tot   = 0;
  apr_finfo_t finfo;
  apr_status_t apr_stat = apr_file_info_get (&finfo, APR_FINFO_SIZE |0, apr_infile_fd);
  if(apr_stat != APR_SUCCESS) {
    assert(0);
  }
  apr_mmap_t *inmmap;
  apr_stat = apr_mmap_create(&inmmap, apr_infile_fd, 0, finfo.size, APR_MMAP_READ |0, pool);
  if(apr_stat != APR_SUCCESS) {
    fprintf(stderr, "apr_mmap_create died at %d!", __LINE__);
    assert(0);
  }

  while(apr_file_eof(apr_infile_fd) != APR_EOF) {
    apr_size_t r = read_size;
    apr_status_t reader = apr_file_read(apr_infile_fd, buf, &r);
    //printf("0=%c 1=%c 2=%c 3=%c\n", buf[0], buf[1], '0' + buf[2], '0' + buf[3]);
    if(r == 0) {
      break;
    }
    if(r != hd_size) {
      printf("r=%zu block_size_tot=%zu\n", r, block_size_tot);
      assert(0);
    }
    dach_header_unpack(hd1, buf);
    u8 id1 = dach_header_get_u8(hd1, "id1");
    u8 id2 = dach_header_get_u8(hd1, "id2");
    u8 maj = dach_header_get_u8(hd1, "major");
    u8 min = dach_header_get_u8(hd1, "minor");
    if(id1 == 'H' && id2 == 'J') {
      u32 block_size = dach_header_get_u32(hd1, "block_size");
      u32 crc        = dach_header_get_u32(hd1, "crc");
      printf("%zu: m=%c%c v=%u.%u block_size=%u crc=%u\n", bid, id1, id2, maj, min, block_size, crc);
      bid++;
      seek_where += block_size + hd_size;
      block_size_tot += block_size + hd_size;
      apr_off_t pos = seek_where;
      apr_status_t seek_stat = apr_file_seek(apr_infile_fd, APR_SET, &pos);
      if(pos != seek_where) {
        printf("pos=%lld seek_where=%lld bid=%lu\n", pos, seek_where, bid);
      }
    }
    else {
      perror("Fatal: Looks like a corrupted file\n");
      break;
    }

  }
  /*
   dach_stream *stream = dach_stream_open(pool, apr_infile_fd);
   dach_stream_ops *s_ops = stream->s_ops;
   dach_block_ops *b_ops = s_ops->block_ops(stream);

   size_t bid = 0;
   while (s_ops->blocks(stream)) {

   dach_block *blk = s_ops->get_block(stream, bid++);
   if (blk != NULL) {
			u32 pos = 0;
			const u32 bsize = b_ops->length(blk);
   }
   }
   */
  apr_file_close(apr_infile_fd);
  apr_pool_destroy(pool);
  return 0;
}




  /*
   * static int enc_stream_working(const char *infile) {
   * assert(0); int thread_count = 1; size_t filesize =
   * dach_io_file_size(infile); const char *outfile =
   * "test_outfile.arc"; dach_stream     *stream =
   * dach_stream_new(infile, outfile, DACH_BLOCK_SIZE_HINT);
   * dach_stream_ops *s_ops  = stream->s_ops; dach_block_ops  *b_ops
   * = s_ops->block_ops(stream);
   * 
   * u32 bid = 0; while(s_ops->blocks(stream) > 0) { dach_block *blk
   * = s_ops->new_block(stream, bid); //fprintf(stderr,
   * "block=%u\n", bid); bid++; const u32 bsize =
   * b_ops->length(blk); if (bsize > 0) { dach_ppm_model *mod =
   * dach_ppm0_new_(); ARC *arc = dach_new_arcoder_(mod, blk); u32
   * pos = 0; while(pos < bsize) { u8 c = b_ops->get_c(blk);
   * //fprintf(stdout, "%c", c); dach_encode(arc, c); pos++; }
   * //fprintf(stderr, "pos=%u\n", pos); dach_stop_encoder(arc);
   * dach_delete_arcoder(arc); dach_ppm0_free_(mod);
   * s_ops->free_block(stream, blk); } } dach_stream_free(stream);
   * return 1; }
   */

  /*
   * static int foo__decode(apr_pool_t * pool, apr_file_t * fd,
   * const char *fname) { int fd_mmap_in = open(fname,
   * O_RDONLY); if (fd_mmap_in == -1) { perror("Error opening
   * file for reading"); exit(EXIT_FAILURE); } size_t fsize =
   * dach_io_file_size(fname); char *data_in = mmap(0, fsize,
   * PROT_READ, MAP_SHARED, fd_mmap_in, 0); if (data ==
   * MAP_FAILED) { close(fd_mmap_in); perror("Error mmapping
   * the file"); exit(EXIT_FAILURE); }
   * 
   * FILE           *fdec; char           *mode = "rb"; fdec =
   * fopen(fname, mode); if (fdec == NULL) { fprintf(stderr,
   * "Can't open input file: %s!\n", fname); exit(1); } char
   * *ext = strrchr(fname, '.'); if (ext == NULL || strcmp(ext,
   * file_ext) != 0) { fprintf(stderr, "Missing file extension
   * on file: %s %s!\n", ext, fname); exit(1); } ext[1] = '1';
   * ext[2] = '\0'; int size = strlen(fname); char
   * *outfilename = malloc(sizeof(char) * size + 1);
   * strcat(outfilename, fname); ext[0] = '.'; fprintf(stderr,
   * "size = %i outfilename=%s newsize = %lu \n", size,
   * outfilename, strlen(outfilename)); FILE *fd_out =
   * fopen(outfilename, "w"); if (!fd_out) { fprintf(stderr,
   * "Fatal: Unable to open file for writing:%s\n",
   * outfilename); exit(1); } dach_ppm_model *mod = dach_ppm0_new_();
   * ARC            *arc = dach_new_arcoder(mod, fdec, stdout,
   * 256, 0); dach_start_decoder(arc); int         i = 0; const
   * u32        max_buf = 4096; u8              buf
   * [max_buf]; u32             p_buf = 0;
   * 
   * while (!h_eof(arc)) { buf[p_buf++] = dach_decode(arc);
   * //fprintf(stderr, "sym = %c\n", sym); if (p_buf ==
   * max_buf) { fwrite(buf, 1, p_buf, fd_out); p_buf = 0; } }
   * fwrite(buf, 1, p_buf, fd_out); fflush(fd_out);
   * fclose(fd_out); dach_delete_arcoder(arc); dach_ppm0_free_(mod);
   * return 0; }
   */

  /*
   * static int slab_file(apr_pool_t * pool, apr_file_t * fd,
   * const char *fname) { assert(fname != NULL); assert(pool !=
   * NULL); struct dach_slab  *slab = dach_new_slab(pool, fd, fname);
   * assert(slab != NULL); char *error;
   * 
   * int                size = strlen(fname) + strlen(file_ext); char
   * *outfilename = malloc(sizeof(char) * size + 1);
   * strcat(outfilename, fname); strcat(outfilename, file_ext);
   * fprintf(stderr, "size = %i outfilename=%s newsize = %lu
   * \n", size, outfilename, strlen(outfilename)); FILE
   * *fd_out = fopen(outfilename, "w"); if (!fd_out) {
   * fprintf(stderr, "Fatal: Unable to open file for
   * writing:%s\n", outfilename); exit(1); } size_t
   * size = slab->elem; size_t          pos = 0; if (fsize >
   * 0) { dach_ppm_model *mod = dach_ppm0_new_(); ARC *arc =
   * dach_new_arcoder(mod, stdin, fd_out, 256, (u64) fsize); for
   * (pos = 0; pos < slab->elem; pos++) { unsigned int  c =
   * slab->slab[pos]; dach_encode(arc, c); } dach_stop_encoder(arc);
   * dach_delete_arcoder(arc); dach_ppm0_free_(mod); }
   * fclose(fd_out); return 0; }
   */

  /*
   * int dach_charclass_file(apr_pool_t * pool, apr_file_t * fd,
   * const char *fname){ assert(fname != NULL); assert(pool !=
   * NULL); struct dach_slab  *slab = dach_new_slab(pool, fd, fname);
   * assert(slab != NULL); assert(slab->elem > 0); size_t
   * size = slab->elem; size_t          pos = 0; int
   * its = 0; int               one = 1; unsigned int   tmp = 0;
   * unsigned int       out = 0; unsigned int   x = 7; for (pos = 0;
   * pos < slab->elem; pos++) { unsigned int    c =
   * slab->slab[pos]; tmp = c & (1 << x); out = (out << 1); if
   * (tmp > 0) { out += 1; } bits++; if (bits == 8) {
   * printf("%c", out); fflush(stdout); bits = 0; tmp = 0; out
   * = 0; } } return 0; }
   */

  /*
   * int dach_unpack_file(apr_pool_t * pool, apr_file_t * fd,
   * const char *fname){ assert(fname != NULL); assert(pool !=
   * NULL); struct dach_slab  *slab = dach_new_slab(pool, fd, fname);
   * assert        (slab != NULL); assert        (slab->elem >
   * 0); size_t         fsize = slab->elem; size_t
   * os = 0; unsigned int       nil = 241; int          start = 0;
   * unsigned int       end = slab->elem - 3; for             (pos =
   * 0; pos < slab->elem; pos++) { unsigned int c =
   * slab->slab[pos]; int               ls = 1; int             ps =
   * 1; unsigned char   c_len = 1; unsigned short int hu_len
   * = 1; unsigned int  u_len = 1;
   * 
   * unsigned char      c_pos = 1; unsigned short int hu_pos = 1;
   * unsigned int       u_pos = 1; //61 62 72 61 f1 00 05 f1 01 03 0
   * a if              (c >= nil) { if (c > 254) {
   * fprintf(stderr, "c=%u\n", c); assert(c == 241); } char
   * *slab_p = (char *)&slab->slab[pos + 1];
   * 
   * if (c == 241) { unsigned char      cp = (unsigned
   * char)slab_p[0]; unsigned char      lp = (unsigned
   * char)slab_p[1]; u_pos = (unsigned int)cp; u_len =
   * (unsigned int)lp; if (u_pos > 255) { fprintf(stderr,
   * "cp=%c lp=%c\n", cp, lp); } assert(u_pos <= 255); ps = 1;
   * ls = 1; } else if (c == 242) { unsigned char       cp1 =
   * (unsigned char)slab_p[0]; unsigned char    cp2 =
   * (unsigned char)slab_p[1]; unsigned char    lp =
   * (unsigned char)slab_p[2]; u_pos = (unsigned int)cp1; u_pos
   * = (u_pos << 8); u_pos += (unsigned int)cp2; u_len =
   * (unsigned int)lp; if (u_len > 255) { fprintf(stderr,
   * "u_pos=%u\n", u_pos); exit(0); } ps = 2; ls = 1; } else if
   * (c == 254) { unsigned char cp0 = (unsigned
   * char)slab_p[0]; unsigned char      cp1 = (unsigned
   * char)slab_p[1]; unsigned char      cp2 = (unsigned
   * char)slab_p[2]; unsigned char      cp3 = (unsigned
   * char)slab_p[3]; unsigned char      lp = (unsigned
   * char)slab_p[4]; u_pos = (unsigned int)cp0; u_pos = (u_pos
   * << 8); u_pos += (unsigned int)cp1; u_pos = (u_pos << 8);
   * u_pos += (unsigned int)cp2; u_pos = (u_pos << 8); u_pos +=
   * (unsigned int)cp3; u_len = (unsigned int)lp; if (u_len >
   * 255) { fprintf(stderr, "u_pos=%u\n", u_pos); exit(0); } ps
   * = 4; ls = 1; } else if (c > 242) { fprintf(stderr,
   * "Exiting, hit a character I don't recognize %u\n", c);
   * exit(0); } unsigned int    max = u_pos + u_len; if (max
   * > slab->elem) { fprintf(stderr, "Exiting, max extend past
   * end of file c=%u u_pos=%u u_len=%u\n", c, u_pos, u_len);
   * exit(0); } unsigned char  *tmp = &slab->slab[u_pos]; int
   * verflow = (u_pos + u_len) - pos; if (overflow > 0) { u_len
   * = pos - u_pos; unsigned int        copy = 0; for (copy = 0; copy
   * < u_len; copy++) { slab->slab[pos + copy] = tmp[copy]; }
   * fwrite(tmp, sizeof(char), u_len, stdout); int
   * em = 0; for (rem = 0; rem < overflow; rem++) { fwrite(tmp,
   * sizeof(char), 1, stdout); } } else { unsigned int  copy
   * = 0; for (copy = 0; copy < u_len; copy++) { slab->slab[pos
   * + copy] = tmp[copy]; } fwrite(tmp, sizeof(char), u_len,
   * stdout); } pos = pos + ps + ls; } else {
   * fwrite(&slab->slab[pos], sizeof(char), 1, stdout); } }
   * return 0; }
   */
/*
static char *new_filename(const char *fname)
{
	assert(fname != NULL);
	size_t s1 = strlen(fname);
	size_t s2 = strlen(const_file_ext);
	s2 += s1;
	char *fout = malloc(sizeof(char) * s2 + 1);
	strcat(fout, fname);
	strcat(fout, const_file_ext);
	return fout;
}

static void free_filename(char *fname)
{
	free(fname);
}
static void *thread_stream(void *v_stream)
{
	assert(0);
	dach_stream *stream = (dach_stream *) v_stream;
	dach_stream_ops *s_ops = stream->s_ops;
	dach_block_ops *b_ops = s_ops->block_ops(stream);

	dach_block *blk = s_ops->new_block(stream, 0);
	const u32 bsize = b_ops->length(blk);
	if (bsize > 0) {
		dach_ppm_model *mod = dach_ppm0_new_();
		ARC *arc = dach_new_arcoder_(mod, blk);
		u32 pos = 0;
		while (pos < bsize) {
			u8 c = b_ops->get_c(blk);
			// fprintf(stdout, "%c", c);
			dach_encode(arc, c);
			pos++;
		}
		dach_stop_encoder(arc);
		dach_delete_arcoder(arc);
		dach_ppm0_free_(mod);
		s_ops->free_block(stream, blk);
	}
	return 0;
}
*/
