#include "dach/dach_ar64.h"
#include "dach/dach_header.h"
#include <assert.h>
#include <stdlib.h>

#include <inttypes.h>

static const u32 DACH_MSB       = 0x80000000; // 2147483648 10000000000000000000000000000000
static const u32 DACH_32B       = 0xFFFFFFFF; // 11111111111111111111111111111111
static const u32 TOP_VALUE      = 0xFFFFFFFF;
static const u32 IO_BUFFER_SIZE = 32;
static const u32 DACH_CHR_COUNT = 257;
static const u32 DACH_SYM_START = 1;
static const u32 DACH_SYM_COUNT = 258;
/*
static const u8  dach_8MSB      = 0x80;       // 128 10000000 
static const u32 dach_2MSB      = 0x40000000; // 1073741824 01- 28 0's -00
static const u32 dach_3MSB      = 0x20000000; // 1073741824 01- 28 0's -00
static const u32 dach_MOFF_2MSB = 0x3FFFFFFF; // 1073741823 00111111111111111111111111111111
static const u32 INC            = 1;
static const u32 dach_1QTR      = TOP_VALUE/4 + 1;//0x40000000;
static const u32 dach_3QTR      = dach_1QTR*3;//0xC0000000;
static const u32 dach_HALF      = dach_1QTR*2;//0x80000000;
static const u32 IO_BUFFER           = 8;
static const u32 dach_RESCALE_FACTOR = 2;
static const u32 dach_RESCALE_MAX    = 0xffff;
*/

typedef struct DACH_AR_CODER {
  dach_ppm_model *mod;
  dach_ppm_api   *api;
  dach_block     *blk;
  dach_block_ops *b_ops;
  FILE *in;
  FILE *out;
  int eof_in;
  int eof_out;
  int stop;
  u32 underflow;
  u64 low;
  u64 high;
  u64 range;
  u64 total_syms;
  u64 total_bits;
  u64 total_bits_in;
  u64 total_bits_out;
  u32 io_buffer;
  u32 io_buffer_full;
  u32 io_buffer_size;
  u32 esym; /* Encoded Symbol taken bit by bit from io_buffer */
  u64 blocksize;
  u64 scale;
  u64 reserved;
} DACH_ARC;

static u64 dach_calc_range(const ARC *arc);
static u64 dach_high_scale(const ARC *arc, const u32 sym);
static u64 dach_low_scale(const ARC *arc, const u32 sym);
static int  dach_bitout(ARC *arc, u8 bit);
static u32  dach_bitin(ARC *arc);
static void dach_encode_step(ARC *arc, u32 sym);
static void dach_write_stream(ARC *arc, u32 bit);
static void dach_dump_arc(const ARC *arc, const char *file, const int line);

/* 
 * The only reason for these non-static getters is to make testing easier
 */

//#define DACH_CHECK_BIT(var,pos) ((var) & ((unsigned)1<<(pos)))
#define DACH_CHECK_BIT(var,pos) ((var) & (1<<(pos)))
//#define HM_PRINT_SYMTAB(A) dach_print_symtab((A), __FILE__, __LINE__)
#define HM_DUMP_ARC(A)     dach_dump_arc((A), __FILE__, __LINE__)

/*
void dach_do_print_symtab(const ARC *arc) {
  HM_PRINT_SYMTAB(arc);
}

u32 dach_do_find_sym(ARC *arc) {
  return dach_find_sym(arc);  
}

u64 dach_get_calc_range(ARC * arc) {
  return dach_calc_range(arc);
}
void dach_do_write_stream(ARC *arc, const u8 bit) {
  dach_write_stream(arc, bit);
}
void dach_do_encode_step(ARC *arc, const u8 sym) {
  dach_encode_step(arc, sym);
}
u32 dach_get_low_scale(const ARC *arc, const u32 sym) {
  return dach_low_scale(arc, sym);
}
u32 dach_get_high_scale(const ARC *arc, const u8 sym) {
  return dach_high_scale(arc, sym);
}
u64 dach_do_get_cumfreq(const ARC *arc, const u32 sym) {
  return arc->api->cumfreq(arc->mod, sym);
}
u32 dach_get_arc_low(ARC *arc) {
  return arc->low;
}
u32 dach_get_arc_high(ARC *arc) {
  return arc->high;
}
u32 dach_get_arc_range(ARC *arc) {
  return arc->range;
}
u32 dach_get_arc_scale(ARC *arc) {
  return arc->scale;
}
u32 dach_get_arc_underflow(ARC *arc) {
  return arc->underflow;
}
u32 dach_get_arc_io_buffer(ARC *arc) {
  return arc->io_buffer;
}
u32 dach_get_arc_io_buffer_full(ARC *arc) {
  return arc->io_buffer_full;
}
u32 dach_get_arc_io_buffer_size(ARC *arc) {
  return arc->io_buffer_size;
}
*/


/*
static void dach_set_cumfreq(const ARC *arc, const u32 sym, const u64 cf) {
  assert(sym < dach_SYM_COUNT);
  u32 chr = arc->sym_to_chr[sym];
  arc->cumfreq[chr] = cf;
}
*/

int dach_eof(ARC *arc) {
  return arc->stop;
}

ARC * dach_new_arcoder_(dach_ppm_model *mod, dach_block *block) {
  ARC *arc = (ARC *) calloc(1, sizeof(ARC));
  arc->mod = mod;
  arc->api = mod->api;
  arc->blk        = block;
  arc->b_ops      = block->ops;
  assert(arc->b_ops != NULL);
  arc->blocksize  = arc->b_ops->length(arc->blk);
  arc->scale      = arc->api->scale(arc->mod);
  assert(arc->scale != 0);
  arc->total_bits_in  = 0;
  arc->total_bits_out = 0;
  arc->stop       = 0;
  arc->in         = NULL;/*TODO Remove in favor of blockio */
  arc->out        = NULL;
  arc->underflow  = 0;
  arc->high       = DACH_32B;
  arc->range      = 0;
  arc->io_buffer  = 0;
  arc->io_buffer_full = IO_BUFFER_SIZE;
  arc->io_buffer_size = IO_BUFFER_SIZE;
  u32 ex = 32;
  //dach_arc_init(arc);
  return arc;
}

ARC * dach_new_arcoder(dach_ppm_model *mod, FILE * in, FILE *out, u32 symcount, u64 blocksize) {
  ARC *arc = (ARC *) calloc(1, sizeof(ARC));
  arc->mod = mod;
  arc->api = mod->api;
  u8 *htmp = calloc(4, sizeof(u8));
  if(blocksize == 0) {
    //Original File size is stored in first 4 bytes.
    u32 tmp = 0;
    int st = fread(htmp, sizeof(u8), 4, in);
    dach_read_hdr_u32(htmp, &tmp);
    //fprintf(stderr, "tmp=%u\n", tmp);
    //fprintf(stderr, "htmp=%u\n", *(u32 *)htmp);
    blocksize = *(u32 *)htmp;
    //fprintf(stderr, "blocksize=%"PRIu64"\n", (u64)blocksize);
  }
  else {
    u32 tmp = blocksize;
    dach_write_hdr_u32(htmp, tmp);
    int st = fwrite(htmp, sizeof(u8), 4, out);
  }
  free(htmp);

  arc->scale = arc->api->scale(arc->mod);
  assert(arc->scale != 0);
  arc->total_bits_in  = 0;
  arc->total_bits_out = 0;
  arc->stop       = 0;
  arc->in         = in;
  arc->out        = out;
  arc->underflow  = 0;
  arc->high       = DACH_32B;
  arc->range      = 0;
  arc->io_buffer  = 0;
  arc->io_buffer_full = IO_BUFFER_SIZE;
  arc->io_buffer_size = IO_BUFFER_SIZE;
  arc->blocksize  = blocksize;
  //arc->freq       = (u32 *) calloc(symcount + ex, sizeof(u32));
  //arc->cumfreq    = (u64 *) calloc(symcount + ex, sizeof(u64));
  //arc->symtable   = (u32 *) calloc(symcount + ex, sizeof(u32));
  //dach_arc_init(arc);
  return arc;
}

void dach_delete_arcoder(ARC * arc) {
   free(arc);
}

static u64 dach_low_scale(const ARC *arc, const u32 sym) {
  assert(sym <= 256);
  assert(sym != 0);
  if(sym == 0) {
    //HM_PRINT_SYMTAB(arc);
    fprintf(stderr, "Error: sym==0 aborting\n");
    exit(1);
  }
  u64 ls6 = arc->low + ((u64)(arc->range * arc->api->cumfreq(arc->mod, sym + 1))/arc->scale);
  u32 ls3 = arc->low + ((u64)(arc->range * arc->api->cumfreq(arc->mod, sym + 1))/arc->scale);
  if(ls6 != ls3) {
    fprintf(stderr, "low_scale ls64=%"PRIu64" == ls32=%u\n", ls6, ls3);
    exit(1);
  } 
  return ls6;
}

static u64 dach_high_scale(const ARC *arc, const u32 sym) {
  assert(sym <= 256);
  assert(arc->scale != 0);
  u64 hs6 = arc->low + ((u64)(arc->range * arc->api->cumfreq(arc->mod, sym))/arc->scale) - 1;
  u32 hs3 = arc->low + ((u64)(arc->range * arc->api->cumfreq(arc->mod, sym))/arc->scale) - 1;
  if(hs6 != hs3) {
    //HM_PRINT_SYMTAB(arc);
    fprintf(stderr, "high_scale sym=%u == hs64=%"PRIu64" == hs32=%u\n",sym, hs6, hs3);
    fprintf(stderr, "( (u64)(arc->range * arc->api->cumfreq(arc->mod, sym + 1))/arc->scale - 1)\n");
    fprintf(stderr, "( (u64)(%"PRIu64" * %"PRIu64")/%"PRIu64" - 1)\n", arc->range, arc->api->cumfreq(arc->mod, sym), arc->scale);
    exit(1);
  } 
  return hs6;
}

static u64 dach_calc_range(const ARC *arc) {
  if(arc->high <= arc->low) {
    fprintf(stderr, "high=%"PRIu64" low=%"PRIu64"\n", arc->high, arc->low);
    exit(1);
  }
  u64 tmp = ((u64) (arc->high - arc->low)) + 1UL;
  assert(tmp != 0);
  return tmp;
}


static void dach_encode_step(ARC *arc, const u32 sym) {
  //fprintf(stderr, "sym=%u\n",sym);
  assert(sym != 0 && sym <= 256);
  assert(arc->high > arc->low);
  arc->range = dach_calc_range(arc);
  if(arc->range == 0) {
    fprintf(stderr, "Error: range == 0\n");
    HM_DUMP_ARC(arc); 
    exit(1);
  }
  u64 hs = dach_high_scale(arc, sym);
  u64 ls = dach_low_scale(arc , sym);
  if(hs < ls) {
    //HM_PRINT_SYMTAB(arc);
    HM_DUMP_ARC(arc);
    fprintf(stderr, "sym=%u range=%"PRIu64" high=%"PRIu64" low=%"PRIu64" scale=%"PRIu64" lcumfreq=%"PRIu64" hcumfreq=%"PRIu64"\n",sym, arc->range, arc->high, arc->low, arc->scale, arc->api->cumfreq(arc->mod, sym), arc->api->cumfreq(arc->mod, sym + 1));
    fprintf(stderr, "hs=%"PRIu64" < ls=%"PRIu64"\n",hs, ls);
    exit(1);
  }
  arc->high = hs;   
  arc->low  = ls;
}



int dach_decode(ARC *arc) {
  assert(arc->high > arc->low);
  arc->range = dach_calc_range(arc);
  u64 cumfreq = (((u64)(arc->esym - arc->low) + 1UL)*arc->scale - 1)/arc->range;
  u32 sym = arc->api->find_sym(arc->mod, cumfreq);
  arc->total_syms++;
  assert(arc->total_syms <= arc->blocksize);
  if(arc->total_syms == arc->blocksize) {
    u8 chr = arc->api->sym_to_chr(arc->mod, sym);
    //HM_PRINT_SYMTAB(arc);
    //fprintf(stderr, "sym=%u chr=%c chr=%u arc->total_syms=%"PRIu64" == arc->blocksizesym==%"PRIu64"\n", sym, chr, chr, arc->total_syms, arc->blocksize);
    arc->stop = 1;
    return chr;
  }
  assert(arc->high > arc->low);
  if(sym >= 256) {
    fprintf(stderr, "sym=%u arc->total_syms=%"PRIu64" == arc->blocksize==%"PRIu64"\n", sym, arc->total_syms, arc->blocksize);
  }
  assert(sym <= 256);
  dach_encode_step(arc, sym);
  assert(arc->api->cumfreq(arc->mod, sym) <= arc->scale); 
  u64 loop = 0; 
  for(;;loop++) {
    assert(arc->high > arc->low);
    if((arc->high & 0x80000000) == (arc->low & 0x80000000)) {
    }
    else if((arc->low & 0x40000000) && !(arc->high & 0x40000000)) {
        arc->esym ^= 0x40000000;
        arc->low  &= 0x3FFFFFFF;
        arc->high |= 0x40000000;
        assert(arc->high > arc->low);
    }
    else {
      break;
    }
    arc->low  = ( arc->low  << 1)      & 0xFFFFFFFF;
    arc->high = ((arc->high << 1) | 1) & 0xFFFFFFFF;
    assert(arc->high > arc->low);
    arc->esym <<= 1;
    arc->esym &= 0xFFFFFFFF;
    arc->esym |= dach_bitin(arc);
  }
  u32 sym1 = arc->api->sym_to_chr(arc->mod, sym);
  arc->scale = arc->api->update(arc->mod, sym);
  //fprintf(stderr, "sym=%u sym1=%u sym2=%u\n ", sym, sym1, sym2);
  assert(arc->high > arc->low);
  return sym1;//h_sym_to_chr(arc->mod, sym);
}

int dach_encode(ARC *arc, const u32 chr) {
  //fprintf(stderr, "chr1=%c\n ", chr);
  const u32 sym = arc->api->chr_to_sym(arc->mod, chr);
  //printf("chr2=%i\n ", (int) chr);
  dach_encode_step(arc, sym);
  arc->total_syms++; //We encode on extra character more than blocksize
  for(;;) {
    if((arc->high & 0x80000000)==(arc->low & 0x80000000)) {   
      dach_write_stream(arc, arc->high >> 31);
    }   
    else if((arc->low & 0x40000000) && !(arc->high & 0x40000000)) {   
        arc->underflow++;
        arc->low  &= 0x3FFFFFFF;
        arc->high |= 0x40000000;
    }   
    else {
      break;
    }
    arc->low  = ( arc->low  << 1)      & 0xFFFFFFFF;
    arc->high = ((arc->high << 1) | 1) & 0xFFFFFFFF;
  }
  arc->scale = arc->api->update(arc->mod, sym);
  return 0;
}

static void dach_write_stream(ARC *arc, u32 bit) {
  //fprintf(stderr, "h_write_stream(%u)\n", bit);
  assert(bit == 1 || bit == 0);
  dach_bitout(arc, bit);
  while(arc->underflow > 0) {
    dach_bitout(arc, ! bit);   
    arc->underflow--;
  }
} 

static int dach_bitout(ARC *arc, u8 bit) {
  int rv = 0;
  //fprintf(stderr, "h_bitout(%u) total_bits=%"PRIu64"\n", bit, arc->total_bits);
  if(arc->io_buffer_full == 0) {
    arc->io_buffer_full = arc->io_buffer_size;
    if(arc->out == NULL) {
      int st = arc->b_ops->write(arc->blk, &arc->io_buffer, 4);
    }
    else {
      size_t n = fwrite(&arc->io_buffer,sizeof(u8),4,arc->out);
    }
    //fprintf(stderr, "writing integer = %u\n", arc->io_buffer);
    //fflush(arc->out);
    arc->io_buffer = 0;
  }
  arc->total_bits_out++;
  arc->io_buffer <<= 1;
  arc->io_buffer += bit;
  arc->io_buffer_full--;
  return rv;
}

static u32 dach_bitin(ARC *arc) {
  //fprintf(stderr, " io_buffer_full=%u\n", arc->io_buffer);
  if(arc->io_buffer_full == 0) {
    assert(!arc->eof_in);
    arc->io_buffer_full = arc->io_buffer_size;
    //XXX IO Routine needed here!
    assert(sizeof(arc->io_buffer_full) == 4);
    arc->io_buffer = 0;
    if(arc->out == NULL) {
      /**\todo 
       what do we do here when we read less than the amount asked for?
       We should return the amount we got and adjust the 
       io_buffer_full to the (bytes_read * 8)
       */
      int st = arc->b_ops->read(arc->blk, &arc->io_buffer, 4);
    }
    else {
      /*
       * fread is slow , the fewer calls here the better.
       */
      size_t n = fread(&arc->io_buffer,sizeof(u8),4,arc->in);
      if (feof(arc->in)) {
        arc->eof_in = 1;
      }
    }
  }
  arc->total_bits_in++;
  u32 t = (u32)arc->io_buffer & (u32)DACH_MSB;
  t >>= 31;
  assert(t == 0 || t == 1);
  arc->io_buffer <<= 1;
  arc->io_buffer_full--;
  return t;
}


int dach_stop_encoder(ARC *arc) {
  assert(arc->total_syms == arc->blocksize);
  const u32 low = (u32) arc->low & 0xFFFFFFFF;
  u64 tmp = DACH_CHECK_BIT(low, 31);
  assert(tmp == 1 || tmp == 0);
  arc->underflow++;
  dach_write_stream(arc, !tmp);
  dach_write_stream(arc, tmp);
  while(arc->io_buffer_full != 0) {
    dach_write_stream(arc, tmp);
  }
  dach_write_stream(arc, tmp);
  return 0;
}

int dach_start_decoder(ARC *arc) {
  int i = 0;
  arc->io_buffer_full = 0;
  for(i = 0 ; i < 32; i++) {
    arc->esym *= 2;
    arc->esym += dach_bitin(arc); 
  }
  //HM_PRINT_SYMTAB(arc);
  //fprintf(stderr, "esym=%u\n", arc->esym);
  return 0;
}

static void dach_dump_arc(const ARC *arc, const char *file, const int line) {
  fprintf(stderr,"[arc->total_syms == %"PRIu64"]\n", arc->total_syms);
  fprintf(stderr,"[arc->scale      == %"PRIu64"]\n", arc->scale);
  fprintf(stderr,"[arc->low        == %"PRIu64"]\n", arc->low);
  fprintf(stderr,"[arc->esym       == %u]\n", arc->esym);
  fprintf(stderr,"[arc->high       == %"PRIu64"]\n", arc->high);
  fprintf(stderr,"[arc->range      == %"PRIu64"]\n", arc->range);
  fprintf(stderr,"[arc->underflow  == %u]\n", arc->underflow);
  fprintf(stderr,"[arc->io_buffer       == %u]\n", arc->io_buffer);
  fprintf(stderr,"[arc->io_buffer_full  == %u]\n", arc->io_buffer_full);
  fprintf(stderr,"[arc->io_buffer_size  == %u]\n", arc->io_buffer_size);
  fprintf(stderr,"[arc->total_bits      == %"PRIu64"]\n", arc->total_bits);
  fprintf(stderr,"[arc->total_bits_in   == %"PRIu64"]\n", arc->total_bits_in);
  fprintf(stderr,"[arc->total_bits_out  == %"PRIu64"]\n", arc->total_bits_out);
  //fprintf(stderr,"[%u == tarc->io_buffer_size]\n", tarc->io_buffer_size);
  //  fprintf(stderr,"[%u == tarc->io_buffer_full]\n", tarc->io_buffer_full);
  fprintf(stderr, "(((u64)(arc->esym - arc->low))*arc->scale)/arc->range\n");   
  fprintf(stderr, "arc->high=%"PRIu64" TOP_VALUE=%u\n", arc->high, TOP_VALUE);   
  fprintf(stderr, "encode(%u - %"PRIu64"))*%"PRIu64")/%"PRIu64"\n",arc->io_buffer , arc->low, arc->scale, arc->range);   
  fprintf(stderr, "decode(%u - %"PRIu64"))*%"PRIu64")/%"PRIu64"\n",arc->esym , arc->low, arc->scale, arc->range);   
  u64 cumfreq = (((u64)(arc->esym - arc->low))*arc->scale)/arc->range;
  fprintf(stderr, "cumfreq(%"PRIu64")\n", cumfreq);   
  fprintf(stderr, "sym=0   arc->api->cumfreq(arc->mod, 0)=%"PRIu64"\n", arc->api->cumfreq(arc->mod, 0));
  fprintf(stderr, "sym=1   arc->api->cumfreq(arc->mod, 1)=%"PRIu64"\n", arc->api->cumfreq(arc->mod, 1));
  fprintf(stderr, "sym=2   arc->api->cumfreq(arc->mod, 2)=%"PRIu64"\n", arc->api->cumfreq(arc->mod, 2));
  fprintf(stderr, "sym=255 arc->api->cumfreq(arc->mod, 255)=%"PRIu64"\n", arc->api->cumfreq(arc->mod, 255));
  /*
  u64 hscale = dach_high_scale(arc, arc->sym);//->low + (u64)(arc->range * arc->api->cumfreq(arc->mod, '1'))/arc->scale) - 1;
  u64 lscale = dach_low_scale(arc, arc->sym); //( arc->low + (u64)(arc->range * arc->api->cumfreq(arc->mod, '1' - 1))/arc->scale) + 1;
  fprintf(stderr, "hcumfreq=%"PRIu64" lcfreq=%"PRIu64"\n",arc->api->cumfreq(arc->mod, arc->sym), arc->api->cumfreq(arc->mod, arc->sym - 1));
  fprintf(stderr, "hscale=%"PRIu64" lscale=%"PRIu64" calc_range=%"PRIu64" range=%"PRIu64" high=%"PRIu64" low=%"PRIu64" scale=%"PRIu64"\n", 
    hscale, lscale, hscale - lscale, arc->range, arc->high, arc->low, arc->scale);
  fprintf(stderr, "h_dump_arc in file=%s line=%i\n", file, line);
  */
} 
