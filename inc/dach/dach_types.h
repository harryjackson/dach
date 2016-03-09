#ifndef DACH_TYPES_H
#define DACH_TYPES_H
#include "dach_class.h"
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef enum DACH_MODE {
  DACH_STREAM_INVALID   = 0, //Runtime Exception
  DACH_STREAM_READABLE,
  DACH_STREAM_WRITABLE,
  DACH_STREAM_READWRITE,
} DACH_STREAM_MODE;

typedef enum DACH_TYPE {
  DACH_TYPE_UNKNOWN = 0, // Runtime Exception
  DACH_TYPE_BLOCK,  //stream block
  DACH_TYPE_STREAM, //stream stream
  DACH_TYPE_OPERATION,
  DACH_TYPE_PPM0_ENC, //encoder
  DACH_TYPE_COPY_ENC, //encoder
} DACH_TYPE;

struct DACH_TYPE_INDEX {
  const DACH_TYPE type;
  const char *name;
};

static struct DACH_TYPE_INDEX dt_index[] = {
  { DACH_TYPE_UNKNOWN  , "Unkown or invalid type"},
  { DACH_TYPE_BLOCK    , "Block type"},
  { DACH_TYPE_STREAM   , "Stream type"},
  { DACH_TYPE_PPM0_ENC , "PPM0 Model type"},
};
#endif /* DACH_TYPES_H */
