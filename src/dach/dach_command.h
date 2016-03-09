#ifndef DACH_COMMAND_H
#define DACH_COMMAND_H
#include "dach/dach_io.h"
#include "dach/dach_stream.h"
#include <stdio.h>

static const char cmd_usage_string[] =
"\tdach <cmd> <args>\n"
"\t\tdach copy    --file <fname>\n"
"\t\tdach encode  --file <fname>\n"
"\t\tdach decode  --file <fname.dch>\n"
"\t\tdach estream --file <fname>\n"
"\t\tdach dstream --file <fname.dch>\n"
"\t\tdach info    --file <fname.dch>\n";

int dach_cmd_copy(DACH_CONFIG *dc);
int dach_cmd_decode(DACH_CONFIG *dc);
int dach_cmd_encode(DACH_CONFIG *dc);
int dach_cmd_estream(DACH_CONFIG *dc);
int dach_cmd_dstream(DACH_CONFIG *dc);
int dach_cmd_info(DACH_CONFIG *dc);
int dach_get_arg(const int argc, const char *const argv[], const char *opt);
#endif /* dach_COMMAND_H */ 
