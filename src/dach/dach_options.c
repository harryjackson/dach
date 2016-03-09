#include "dach_options.h"
#include <string.h>

int dach_is_opt_set(const int argc, const char* argv, const char* opt) {
  int tmp = 1;
  for(tmp = 1; tmp <= argc ;tmp++) {
    strstr(&argv[tmp], opt);
  }
  if(tmp == argc) {
    return 0;
  }
  return 1;
}

void dach_vreportf(const char *prefix, const char *err, va_list params)
{
  char msg[4096];
  vsnprintf(msg, sizeof(msg), err, params);
  fprintf(stderr, "%s%s\n", prefix, msg);
}

static void dach_usage_builtin(const char *err, va_list params)
{
  dach_vreportf("usage : ", err, params);
  exit(129);
}

void dach_usagef(const char *err, ...)
{
  va_list params;
  va_start(params, err);
  dach_usage_builtin(err, params);
  va_end(params);
}

void dach_usage(const char *err)
{
  dach_usagef("%s", err);
}
