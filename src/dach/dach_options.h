#ifndef DACH_OPTIONS_H
#define DACH_OPTIONS_H
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
typedef struct dach_options     dach_options;
typedef struct dach_options_ops {
  size_t (*size         )();
  int    (*num_threads  )();
  char   (*file         )();
  size_t (*block_size   )();
} dach_options_ops;

struct dach_options {
  dach_options_ops *op;
  void *priv;
};
static dach_options_ops *options_ops   ();
*/



int dach_is_opt_set(const int argc, const char* argv, const char* opt);
void dach_vreportf(const char *prefix, const char *err, va_list params);
void dach_usagef(const char *err, ...);
void dach_usage(const char *err);
#endif /* DACH_OPTIONS_H */
