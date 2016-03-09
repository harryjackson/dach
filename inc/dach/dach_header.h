#ifndef DACH_HEADER_H
#define DACH_HEADER_H
#include "dach_types.h"
#include <stdio.h>
/*
#include <assert.h>
*/
typedef struct dach_header *dach_header;
size_t dach_header_size(void);

dach_header dach_header_new(u32 block_size, u32 crc32, u32 model);
dach_header dach_header_new_blank(void);
dach_header dach_header_unpack(dach_header hdr, u8 *buff);


void dach_header_free(dach_header hdr);
int  dach_header_pack(dach_header hdr, u8 *packed);
int dach_write_hdr_u32(u8 *header, u32 data);
int dach_read_hdr_u32(u8 *header, u32 *data);

u8 dach_header_get_u8(dach_header hdr, const char *field);
u32 dach_header_get_u32(dach_header hdr, const char *field);


//size_t dach_header_size_unpacked(void);
//dach_header dach_header_fread(FILE *fd);
//int dach_write_hdr(u8 *header, u8 *data, u8 bytes);
#endif /* DACH_HEADER_H */
