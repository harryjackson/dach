#ifndef DACH_H
#define DACH_H
#include <stddef.h>
#include "dach_config.h"

void dach_sleep(int sec, int usec);

/*
 * These function are only here because we're using APR
 */
int  dach_app_initialize(int *argc, char const *const **argv, char const *const **env);
int  dach_initialize(void);
void dach_terminate(void);


int dach_copy_file(DACH_CONFIG *dc);
int dach_decode_file(DACH_CONFIG *dc);
int dach_encode_file(DACH_CONFIG *dc);
int dach_estream_file(DACH_CONFIG *dc);
int dach_dstream_file(DACH_CONFIG *dc);
int dach_info_file(DACH_CONFIG *dc);


#endif /* DACH_H */
