#include "dach_command.h"
#include "dach_options.h"
#include "dach_cmdline.h"
#include <string.h>


#define DACH_M_ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define DACH_CMD_OPTION 1

static DACH_CONFIG dach_default_config = {
  NULL,
  "r",
  4,
  8192 * 2,
};

struct cmd_struct {
  const char *cmd;
  int (*cmd_func)(DACH_CONFIG *);
  int option;
};

static struct cmd_struct commands[] = {
  { "encode"    , dach_cmd_encode   , DACH_CMD_OPTION },
  { "decode"    , dach_cmd_decode   , DACH_CMD_OPTION },
  { "estream"   , dach_cmd_estream  , DACH_CMD_OPTION },
  { "dstream"   , dach_cmd_dstream  , DACH_CMD_OPTION },
  { "info"      , dach_cmd_info     , DACH_CMD_OPTION },
};

static int cmd_is_builtin(const char *s)  
{
  size_t i;
  if(s == NULL) {
    return 0;
  }
  for (i = 0; i < DACH_M_ARRAY_SIZE(commands); i++) {
    struct cmd_struct *p = commands+i;
    if (!strcmp(s, p->cmd))
      return 1;
  }
  return 0;
}

static int cmd_run(const char * cmd, DACH_CONFIG *dc) {
  if(! cmd_is_builtin(cmd)) {
    dach_usage(cmd_usage_string);
    exit(1);
  }

  unsigned int i = 0;
  for (i = 0; i < DACH_M_ARRAY_SIZE(commands); i++) {
    struct cmd_struct *p = commands+i;
    if (!strcmp(cmd, p->cmd)) {
      return p->cmd_func(dc);
    }
  }
  return 0;
}

int main(int argc, const char* const argv[]) 
{
  char *ar[argc];
  int i = 0;
  for(i = 0; i < argc; i++) {
    char *d = strdup(argv[i]);
    if(d == NULL) {
      fprintf(stderr, "Fatal Error using strdup\n");
      exit(1);
    }
    ar[i] = d;
  }
  char **av = ar;
  struct gengetopt_args_info ai;
  if (cmdline_parser(argc, av, &ai) != 0) {
    printf("cmdline_parser failed\n");
    exit(1);
  }

  if(ai.file_given) {
    dach_default_config.file = ai.file_arg;
    printf("file=%s\n", dach_default_config.file);
  }

  dach_default_config.threads =
      (ai.threads_given && ai.threads_arg > dach_default_config.threads)
  ? ai.threads_arg
  : dach_default_config.threads;

  dach_default_config.block_size =
      (ai.block_size_given && ((size_t)ai.block_size_arg) > dach_default_config.block_size)
  ? ai.block_size_arg
  : dach_default_config.block_size;

  dach_app_initialize(&argc, &argv, NULL);
  const char * cmd = argv[1];
  int ret = 0;
  if(cmd_is_builtin(cmd)) {
    ret = cmd_run(cmd, &dach_default_config);
  }
  else {
    dach_usage(cmd_usage_string);
    ret = 1;
  }
  //dach_terminate(); //calls apr_terminate();
  return ret;
}
