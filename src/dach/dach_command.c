#include "dach_command.h"
#include <string.h>
//static int dach_test_file_argument(int argc, const char * const argv[]);

int dach_cmd_copy(DACH_CONFIG *dc) {
  return dach_copy_file(dc);
}

int dach_cmd_dstream(DACH_CONFIG *dc) {
  return dach_dstream_file(dc);
}

int dach_cmd_estream(DACH_CONFIG *dc) {
  return dach_estream_file(dc);
}

int dach_cmd_decode(DACH_CONFIG *dc) {
  return dach_decode_file(dc);
}

int dach_cmd_encode(DACH_CONFIG *dc) {
  return dach_encode_file(dc);
}

int dach_cmd_info(DACH_CONFIG *dc) {
  return dach_info_file(dc);
}

int dach_get_arg(const int argc, const char *const argv[], const char *opt)
{
  int	tmp = 0;
  for (tmp = 0; tmp < argc; tmp++) {
    //printf("argc==%i argv=%s\n", tmp, argv[tmp]);
    char *pos = strstr(argv[tmp], opt);
    if(pos && pos != NULL) {
      if (argv[++tmp] != NULL) {
        return tmp;
      } else {
        return -1;
      }
    }
  }
  return -1;
}

/*
static int dach_test_file_argument(int argc, const char * const argv[])
{
  if(argc == -1) {
    fputs("Need a --file argument that works and is readable\n", stderr);
    dach_usage(cmd_usage_string);
  }
  if(!dach_io_file_exists(argv[argc])) {
    fputs("Need a --file argument that works and is readable\n", stderr);
    dach_usage(cmd_usage_string);
  }
  return 0;
}
*/

/*
int dach_cmd_estream(int argc, const char * const argv[], const char * foo)
{
  int argc_val = dach_get_arg(argc, argv, "--file");
  dach_test_file_argument(argc_val, argv);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  const char *fname = argv[argc_val];
  apr_file_t *fd = NULL;
  int rv = dach_estream_file(pool, fd, fname);
  apr_pool_destroy(pool);
  return rv;
}

int dach_cmd_decode(int argc, const char * const argv[], const char * foo)
{
  int argc_val = dach_get_arg(argc, argv, "--file");
  dach_test_file_argument(argc_val, argv);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  const char *fname = argv[argc_val];
  apr_file_t *fd = NULL;
  int rv = dach_decode_file(pool, fd, fname);
  apr_pool_destroy(pool);
  return rv;
}

int dach_cmd_encode(int argc, const char * const argv[], const char * foo)
{
  int argc_val = dach_get_arg(argc, argv, "--file");
  dach_test_file_argument(argc_val, argv);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  const char *fname = argv[argc_val];
  apr_file_t *fd = NULL;
  int rv = dach_encode_file(pool, fd, fname);
  apr_pool_destroy(pool);
  return rv;
}
*/
/*
int dach_cmd_charclass(int argc, const char * const argv[], const char * foo)
{
  int argc_val = dach_get_arg(argc, argv, "--file");
  dach_test_file_argument(argc_val, argv);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  apr_hasdach_t *ht = apr_hasdach_make(pool);
  const char *fname = argv[argc_val];
  apr_file_t *fd = NULL;
  dach_charclass_file(pool, fd, fname);
  apr_pool_destroy(pool);
  //apr_status_t rv = dach_create_apr_hash(pool, ht, slab, fsize);
  return 1;
}
int dach_cmd_redbit(int argc, const char * const argv[], const char * foo)
{
  int argc_val = dach_get_arg(argc, argv, "--file");
  dach_test_file_argument(argc_val, argv);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  apr_hasdach_t *ht = apr_hasdach_make(pool);
  const char *fname = argv[argc_val];
  apr_file_t *fd = NULL;
  struct dach_suffixarray *sa = dach_new_suffixarray(pool, fd, fname);
  dach_find_dupes(&sa);
  dach_delete_suffixarray(sa);
  apr_pool_destroy(pool);
  //apr_status_t rv = dach_create_apr_hash(pool, ht, slab, fsize);
  return 1;
}
int dach_cmd_pack(int argc, const char * const argv[], const char * foo)
{
  int argc_val = dach_get_arg(argc, argv, "--file");
  dach_test_file_argument(argc_val, argv);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  const char *fname = argv[argc_val];
  apr_file_t *fd = NULL;
  dach_pack_file(pool, fd, fname);
  apr_pool_destroy(pool);
  return 1;
}
int dach_cmd_unpack(int argc, const char * const argv[], const char * foo)
{
  int argc_val = dach_get_arg(argc, argv, "--file");
  dach_test_file_argument(argc_val, argv);
  apr_pool_t *pool = NULL;
  apr_pool_create(&pool, NULL);
  const char *fname = argv[argc_val];
  apr_file_t *fd = NULL;
  int rv = dach_unpack_file(pool, fd, fname);
  apr_pool_destroy(pool);
  return rv;
}
*/
