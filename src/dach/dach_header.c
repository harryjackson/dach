#include "dach/dach_header.h"
#include "dach/dach_mem.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void write_bytes(u8 *to, u8 *from, int n);


static const u32 DACH_HDR_SIZE_PACKED = 16;
static const u8  DACH_HDR_ID1         = 'H'; //Ox48
static const u8  DACH_HDR_ID2         = 'J'; //0x4A
static const u8  DACH_HDR_MAJOR       = 0;
static const u8  DACH_HDR_MINOR       = 1;
static unsigned  get_u32(u8 *buff);

struct dach_header {
  u8  id1;       // 1
  u8  id2;       // 2
  u8  maj;       // 3
  u8  min;       // 4
  u32 block_size;// 8
  u32 crc;       // 12
  u32 model;     // 16
};

//static void write_bytes(u8 *from, u8 *to, int n);

dach_header dach_header_new_blank(void) {
  dach_header h;
  DACH_NEW(h);
  h->id1        = 0;
  h->id2        = 0;
  h->maj        = 0;
  h->min        = 0;
  h->block_size = 0;
  h->crc        = 0;
  h->model      = 0;
  return h;
}

dach_header dach_header_new(u32 bsize, u32 crc, u32 model) {
  dach_header h;
  DACH_NEW(h);
  h->id1        = DACH_HDR_ID1;
  h->id2        = DACH_HDR_ID2;
  h->maj        = DACH_HDR_MAJOR;
  h->min        = DACH_HDR_MINOR;
  h->block_size = bsize;
  h->crc        = crc;
  h->model      = model;
  return h;
}

void dach_header_free(dach_header hdr) {
  DACH_FREE(hdr);
}


u8 dach_header_get_u8(dach_header hdr, const char *field) {
  if (strcmp(field, "id1") == 0) {
    return hdr->id1;
  }
  else if (strcmp(field, "id2") == 0) {
    return hdr->id2;
  }
  else if (strcmp(field, "major") == 0) {
    return hdr->maj;
  }
  else if (strcmp(field, "minor") == 0) {
    return hdr->min;
  }
  perror("Error: Bad field supplied to dach_header_get_u8()\n");
  assert(0);
  return 0;//TODO: Bad
}



u32 dach_header_get_u32(dach_header hdr, const char *field) {
  if (strcmp(field, "block_size") == 0) {
    return hdr->block_size;
  }
  else if (strcmp(field, "crc") == 0) {
    return hdr->crc;
  }
  else if (strcmp(field, "model") == 0) {
    return hdr->model;
  }
  perror("Error: Bad field supplied to dach_header_get_u8()\n");
  assert(0);
  return 0;//TODO: Bad
}

int dach_header_pack(dach_header hdr, u8 *packed) {
  size_t rv = 0;
  u8* bbytes = (u8*)&hdr->block_size;
  u8* cbytes = (u8*)&hdr->crc;
  u8* mbytes = (u8*)&hdr->model;
  packed[0] = hdr->id1;
  packed[1] = hdr->id2;
  packed[2] = hdr->maj;
  packed[3] = hdr->min;
  
  write_bytes(&packed[4] , bbytes, 4);
  write_bytes(&packed[8] , cbytes, 4);
  write_bytes(&packed[12], mbytes, 4);
  return 1;
}


dach_header dach_header_unpack(dach_header hdr, u8 *buff) {
  hdr->id1        = buff[0];
  hdr->id2        = buff[1];
  hdr->maj        = buff[2];
  hdr->min        = buff[3];
  hdr->block_size = get_u32(&buff[4]);
  hdr->crc        = get_u32(&buff[8]);
  return hdr;
}


size_t dach_header_size(void) {
  return DACH_HDR_SIZE_PACKED;
}

static void write_bytes(u8 *to, u8 *from, int n) {
  int i = 0;
  for(i = 0 ; i < n; i++) {
    to[i] = from[i];
  }
}

/*
 size_t dach_header_size_unpacked(void) {
  return sizeof(struct dach_header);
}*/


int dach_write_hdr_u32(u8 *header, u32 data) {
  u8 *t = (u8 * )&data;
  write_bytes(header, t, 4);
  return 1;
}

int dach_read_hdr_u32(u8 *header, u32 *data) {
  u8 *t = (u8 * )data;
  write_bytes(t, header, 4);
  return 1;
}




static u32 get_u32(u8 *bytes) {
    u32 sum = 0;
    sum += bytes[0] | (bytes[1]<<8) | (bytes[2]<<16) | (bytes[3]<<24);
    return sum;
}



/*
dach_header dach_header_fread(FILE *fd) {
  u8 buff[H_HDR_SIZE];
  size_t rv = fread(buff, 1, DACH_HDR_SIZE,fd);
  if(rv != DACH_HDR_SIZE) {
    if(feof(fd) || ferror(fd) ) {
      fprintf(stderr, "Bad or short file passed to dach_header_fread\n");
      exit(1);
    }
  }
  dach_header tmp = dach_header_new_from_buffer(buff);
  assert(tmp->id1 == DACH_HDR_ID1);
  assert(tmp->id2 == DACH_HDR_ID2);
  assert(tmp->maj == DACH_HDR_MAJOR);
  assert(tmp->min == DACH_HDR_MINOR);
  return tmp;
}


int dach_read_hdr_u32(u8 *header, u8 *data, u8 bytes) {
  assert(bytes == 1 || bytes == 2 || bytes == 4 || bytes == 8);
  int i = 0;
  for(i = 0; i < bytes;i++) {
    data[i] = header[i]; 
  }
  return bytes;
}

  rv = fwrite(&hdr->id1, 1, 1, fd);
  if(rv != 1) {
      fprintf(stderr, "Unable to write id1 to header\n");
      exit(1);
  }
  rv = fwrite(&hdr->id2, 1, 1, fd);
  if(rv != 1) {
      fprintf(stderr, "Unable to write id2 to header\n");
      exit(1);
  }
  rv = fwrite(&hdr->maj, 1, 1, fd);
  if(rv != 1) {
      fprintf(stderr, "Unable to write major version number to header\n");
      exit(1);
  }
  rv = fwrite(&hdr->min, 1, 1, fd);
  if(rv != 1) {
      fprintf(stderr, "Unable to write minor version number to header\n");
      exit(1);
  }
  rv = fwrite(&hdr->block_size, 1, 4, fd);
  if(rv != 1) {
      fprintf(stderr, "Unable to write block size to header\n");
      exit(1);
  }
  rv = fwrite(&hdr->crc, 1, 4, fd);
  if(rv != 1) {
      fprintf(stderr, "Unable to write block size to header\n");
      exit(1);
  }
  return hdr;

*/
