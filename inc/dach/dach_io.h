#ifndef DACH_IO_H
#define DACH_IO_H
#include <apr-1/apr_pools.h>
#include <apr-1/apr_file_io.h>

off_t dach_io_file_size(const char *fname);
int    dach_io_file_exists(const char *filename);
int    dach_open_file(apr_pool_t *pool, apr_file_t **file, const char* fname, unsigned short OPTIONS);
#endif /* DACH_IO_H */