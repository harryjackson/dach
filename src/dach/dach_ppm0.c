#include "dach/dach_ppm0.h"
#include "dach/dach_mem.h"
#include <assert.h>
#include <stdio.h>
/*
 * TODO:
 * 1. Add rescale back in
 * 2. Remove redundant functions that just wrap others
 */
#define DACH_M_VOID_TO_PPM0 PPM0 *ppm = (PPM0*)api->priv;
//#define DACH_M_CHECK_BIT(var,pos) ((var) & (1<<(pos)))
//#define HM_PRINT_SYMTAB(A) dach_print_symtab((A), __FILE__, __LINE__)

static u32 find_sym(dach_ppm_model *mod, u64 cumfreq);
static u64 update(dach_ppm_model *mod, const u32 sym);
static u64 scale(dach_ppm_model *mod);
static u64 cumfreq(const dach_ppm_model *mod, const u32 sym);
static u32 chr_to_sym(dach_ppm_model *mod, const u32 chr);
static u32 sym_to_chr(dach_ppm_model *mod, const u32 sym);

static dach_ppm_api api = {
  .find_sym = find_sym,
  .update = update,
  .scale = scale,
  .cumfreq = cumfreq,
  .chr_to_sym = chr_to_sym,
  .sym_to_chr = sym_to_chr, 
};

static const u32 DACH_32B            = 0xFFFFFFFF;
static const int DACH_CHR_COUNT      = 257;
static const int DACH_SYM_START      = 1;
static const u32 DACH_SYM_COUNT      = 258;
static const u32 DACH_RESCALE_MAX    = 0xffff;
static const u32 DACH_RESCALE_FACTOR = 2;

typedef struct DACH_MODEL_PPM0 PPM0;

/*TODO: Remove all of these funciton declarations in favor of the static ones above */
static u32 dach_add_sym_chr(const PPM0 *ppm, u32 sym, u32 chr);
static u32 dach_sym_to_chr(PPM0 *ppm, const u32 sym);
static u64 dach_get_freq(const PPM0 *ppm, const u32 sym);
static u64 dach_ppm0_cumfreq(const PPM0 *ppm, const u32 sym);
static u32 dach_ppm0_find_sym(PPM0 *ppm, const u64 cumfreq);
static u64 dach_ppm0_update(PPM0 *ppm, const u32 sym);
static void dach_inc_freq(const PPM0 *ppm, const u32 sym);
static void dach_set_cumfreq(const PPM0 *ppm, const u32 sym, const u64 cf);
//static void dach_set_freq(const PPM0 *ppm, const u32 sym, const u64 freq);
//static int dach_rescale(PPM0 *ppm);
//static void dach_print_symtab(const PPM0 *ppm, const char *file, const int line);

struct DACH_MODEL_PPM0 {
  u64 *cumfreq;
  u64 total_syms;
  u64 scale;
  int eof_in;
  int eof_out;
  int stop;
  u32 sym;
  u32 sym_prev;
  u32 underflow;
  u32 *chr_to_sym;
  u32 *sym_to_chr;
  u32 *freq;
  int type;
  u32 sym_prox_p;
  u32 *sym_prox;
};

void dach_ppm0_free_(dach_ppm_model *api) {
  PPM0 * ppm = (PPM0*) api->priv;
  printf("dach_ppm0_free_\n");
  free(ppm->freq);
  free(ppm->cumfreq);
  free(ppm->chr_to_sym);
  free(ppm->sym_to_chr);
  free(ppm);
  //DACH_FREE(ppm);
  DACH_FREE(api);
}

dach_ppm_model *dach_ppm0_new_(void) {
  dach_ppm_model *model;
  PPM0 *ppm;
  DACH_NEW(model);
  DACH_NEW(ppm);
  int symcount = 256;
  u32 ex = 32;
  ppm->type       = 0xEFBEADDE;
  ppm->freq       = (u32 *) calloc(symcount + ex, sizeof(u32));
  ppm->cumfreq    = (u64 *) calloc(symcount + ex, sizeof(u64));
  ppm->sym_to_chr = (u32 *) calloc(symcount + ex, sizeof(u32));
  ppm->chr_to_sym = (u32 *) calloc(symcount + ex, sizeof(u32));
  u32 sym = 0;
  u32 chr = 0;
  u64 sum = 0;
  u32 t = dach_add_sym_chr(ppm, 0, DACH_SYM_COUNT);
  for(sym = DACH_SYM_COUNT - 1; sym > 0; sym--) {
    chr = sym - 1;
    u32 t = dach_add_sym_chr(ppm, sym, chr);
    dach_inc_freq(ppm, sym);
    sum += dach_get_freq(ppm, sym);
    dach_set_cumfreq(ppm, sym, sum);
  }
  assert(sum > 255);
  ppm->scale = sum;
  //fprintf(stderr, "cf=%llu\n", dach_ppm0_cumfreq(arc->ppm, 0));
  assert(dach_ppm0_cumfreq(ppm, 0) == 0); 
  assert(dach_ppm0_cumfreq(ppm, 1) == ppm->scale);
  assert(dach_ppm0_cumfreq(ppm, 1) == DACH_SYM_COUNT - 1);
  model->api = &api;
  model->priv = ppm;
  return model;
}

//PPM0 * dach_new_ppm0(FILE * in, FILE *out, u32 symcount, u64 blocksize) {
/*
PPM0 * dach_ppm0_new(void) {
  PPM0 *ppm;
  DACH_NEW(ppm);
  int symcount = 256;
  u32 ex = 32; 
  ppm->freq       = (u32 *) calloc(symcount + ex, sizeof(u32));
  ppm->cumfreq    = (u64 *) calloc(symcount + ex, sizeof(u64));
  ppm->sym_to_chr = (u32 *) calloc(symcount + ex, sizeof(u32));
  ppm->chr_to_sym = (u32 *) calloc(symcount + ex, sizeof(u32));
  u32 sym = 0;
  u32 chr = 0;
  u64 sum = 0;
  u32 t = dach_add_sym_chr(ppm, 0, DACH_SYM_COUNT);
  for(sym = DACH_SYM_COUNT - 1; sym > 0; sym--) {
    chr = sym - 1;
    u32 t = dach_add_sym_chr(ppm, sym, chr);
    dach_inc_freq(ppm, sym);
    sum += dach_get_freq(ppm, sym);
    dach_set_cumfreq(ppm, sym, sum);
  }
  assert(sum > 255);
  ppm->scale = sum;
  //fprintf(stderr, "cf=%llu\n", dach_ppm0_cumfreq(arc->ppm, 0));
  assert(dach_ppm0_cumfreq(ppm, 0) == 0); 
  assert(dach_ppm0_cumfreq(ppm, 1) == ppm->scale);
  assert(dach_ppm0_cumfreq(ppm, 1) == DACH_SYM_COUNT - 1);
  return ppm;
}
*/

static u64 find  = 0;
static u64 times = 0;
static u64 find_bin  = 0;
static u64 times_bin = 0;

static u32 find_sym(dach_ppm_model *api, u64 cumfreq) {
  PPM0 *ppm = (PPM0 *) api->priv;
  return dach_ppm0_find_sym(ppm, cumfreq);
}

static u64 cumfreq(const dach_ppm_model *api, const u32 sym) {
  PPM0 *ppm = (PPM0 *) api->priv;
  return dach_ppm0_cumfreq(ppm, sym);
}

static u64 scale(dach_ppm_model *api) {
  PPM0 * ppm = (PPM0*) api->priv;
  return ppm->scale;
}

static u32 sym_to_chr(dach_ppm_model *api, const u32 sym) {
  assert(sym <= DACH_SYM_COUNT);
  DACH_M_VOID_TO_PPM0;
  return ppm->sym_to_chr[sym];
}

static u32 dach_sym_to_chr(PPM0 *ppm, const u32 sym) {
  assert(sym <= DACH_SYM_COUNT);
  return ppm->sym_to_chr[sym];
}

static u32 dach_add_sym_chr(const PPM0 *ppm, u32 sym, u32 chr) {
  assert(chr <= DACH_SYM_COUNT);
  assert(sym < DACH_SYM_COUNT);
  ppm->sym_to_chr[sym] = chr;
  ppm->chr_to_sym[chr] = sym;
  return sym;
}

static u32 chr_to_sym(dach_ppm_model *api, const u32 chr) {
  //fprintf(stderr, "Error: %u\n", chr);
  assert(chr <= 255);
  PPM0 *ppm = (PPM0 *) api->priv;
  return ppm->chr_to_sym[chr];
}

/*
static void dach_set_freq(const PPM0 *ppm, const u32 sym, const u64 freq) {
  u32 chr = ppm->sym_to_chr[sym];
  ppm->freq[chr] = freq;
}
*/

u64 dach_get_freq(const PPM0 *ppm, const u32 sym) {
  u32 chr = ppm->sym_to_chr[sym];
  return ppm->freq[chr];
}

void dach_inc_freq(const PPM0 *ppm, const u32 sym) {
  u32 chr = ppm->sym_to_chr[sym];
  ppm->freq[chr]++;
}

static void dach_inc_cumfreq(const PPM0 *ppm, const u32 sym) {
  u32 chr = ppm->sym_to_chr[sym];
  ppm->cumfreq[chr]++;
}

void dach_set_cumfreq(const PPM0 *ppm, const u32 sym, const u64 cf) {
  assert(sym < DACH_SYM_COUNT);
  u32 chr = ppm->sym_to_chr[sym];
  ppm->cumfreq[chr] = cf;
}

static u64 dach_ppm0_cumfreq(const PPM0 *ppm, const u32 sym) {
  assert(sym <= DACH_SYM_COUNT);
  if(sym == DACH_SYM_COUNT) {
    return 0;
  }
  u32 chr = ppm->sym_to_chr[sym];
  u64 freq = ppm->cumfreq[chr];
  return freq;
}



static u32 dach_ppm0_find_sym(PPM0 *ppm, u64 cumfreq) {
  //ppm->range = dach_calc_range(ppm);
  //u64 cumfreq = (((u64)(ppm->esym - ppm->low) + 1UL)*ppm->scale - 1)/ppm->range;
  u32 sym = 0;
  //u32 loop     = 0;
  //times++;
  //fprintf(stderr, "h_ppm0_cumfreq==%llu, cfreq=%llu, sym=%u\n", dach_ppm0_cumfreq(arc, sym), cumfreq, sym);
  for(sym = 1; sym < DACH_SYM_COUNT && dach_ppm0_cumfreq(ppm, sym) > cumfreq; sym++) {
  //for(sym = DACH_SYM_COUNT - 1; cumfreq < dach_ppm0_cumfreq(arc, sym) && sym != 0; sym--) {
    //fprintf(stderr, "h_find_sym: SEARCHING:cumfreq=%llu/sym=%u\n", cumfreq, sym);
    //loop++;
  }
  //fprintf(stderr, "h_find_sym: FOUND:     dach_ppm0_cumfreq==(%llu) > cumfreq=%llu for sym=%u\n", dach_ppm0_cumfreq(arc, sym), cumfreq, sym);
  sym--;
  //fprintf(stderr, "h_find_sym: FOUND:     dach_ppm0_cumfreq==(%llu) > cumfreq=%llu for sym=%u\n", dach_ppm0_cumfreq(arc, sym), cumfreq, sym);
  //HM_PRINT_SYMTAB(arc);
  //exit(0);
  /*
  find += loop;
  if(find > 11500000) {
     fprintf(stderr, "avgfind=%llu\n", find/times);
     times = 0;
     find  = 0;
  }
  */
  //exit(0);
  if(sym == 0) {
    ppm->sym = sym;
    fprintf(stderr, "cfreq=%llu sym=0\n", (long long unsigned) cumfreq);
    fprintf(stderr, "h_ppm0_cumfreq==%llu, cfreq=%llu, sym=%u\n", (long long unsigned)dach_ppm0_cumfreq(ppm, sym), (long long unsigned)cumfreq, (unsigned) sym);
    assert(0);
  }
  return sym;
}

static u32 dach_sym_swap(PPM0 *ppm, u32 sym1, u32 sym2) {
  u32 sym_ret = sym1;
  u32 chr1  = dach_sym_to_chr(ppm, sym1);
  u32 chr2  = dach_sym_to_chr(ppm, sym2);
  u64 freq1 = ppm->freq[chr1];
  u64 freq2 = ppm->freq[chr2];
  if(freq1 > freq2) {
    ppm->sym_to_chr[sym1] = chr2;
    ppm->sym_to_chr[sym2] = chr1;
    ppm->chr_to_sym[chr1] = sym2;
    ppm->chr_to_sym[chr2] = sym1;
    u64 cfreq1 = ppm->cumfreq[chr1];
    u64 cfreq2 = ppm->cumfreq[chr2];
    ppm->cumfreq[chr1] = cfreq2;
    ppm->cumfreq[chr2] = --cfreq1;
    sym_ret = sym2;
    u64 sym_ = 0;
    u64 sum  = 0;
    u64 freq  = 0;
    if(sym2 > 1) {
      dach_sym_swap(ppm, sym2, sym2 - 1);
    }

  }
  return sym_ret;
}

/*
static int dach_rescale(PPM0 *ppm) {
  u64 freq = 0;
  u64 sum  = 0;
  u32 sym_ = 0;
  for(sym_ = DACH_SYM_COUNT - 1; sym_ > 0; sym_--) {
    freq = dach_get_freq(ppm, sym_);
    freq = freq/H_RESCALE_FACTOR + 1;
    dach_set_freq(ppm, sym_, freq);
    sum += freq;
    dach_set_cumfreq(ppm, sym_, sum);
  }
  return 0;
}
*/



static u64 update(dach_ppm_model *api, const u32 sym) {
  PPM0 *ppm = (PPM0 *) api->priv;
  return dach_ppm0_update(ppm, sym);
}

u64 dach_ppm0_update(PPM0 *ppm, const u32 sym) {
  assert(sym > 0 && sym < DACH_SYM_COUNT);
  int rv = 0;
  u32 sym_test = sym;
  u32 sym_     = sym;
  u64 sum  = 0;
  u64 freq = 0;

 // if(ppm->scale >= dach_RESCALE_MAX) {
    //h_rescale(ppm);
  //}
  dach_inc_freq(ppm, sym);
  if(sym > 1) {
    dach_sym_swap(ppm, sym, sym - 1); 
  }
  //assert(f_test_before == f_test_after);
  sum = 0;


  /*
  u64 tmp_f = dach_get_freq(ppm, sym);
  u32 old_sym = ppm->sym_prox[ppm->sym_prox_p];
  ppm->sym_prox[ppm->sym_prox_p] = sym;
  int seen = 0;
  if(tmp_f > 5) {
    dach_set_freq(arc, sym    , (tmp_f + 2) + 1);
    dach_set_freq(arc, old_sym, (tmp_f - 2) + 1);
    ppm->sym_prox_p++;
    if(ppm->sym_prox_p > 5) {
      ppm->sym_prox_p = 0;
    }
  }
  */

  //for(sym_ = DACH_SYM_COUNT - 1; sym_ > 0; sym_--) {
  for(sym_ = sym; sym_ > 0; sym_--) {
    //tmp_f = dach_get_freq(arc, sym_);
    /*
    if(tmp_f > 5 &&  ! seen ) {
      seen++;
      if(sym_ == ppm->sym_prev) {
        dach_set_freq(arc, sym_, (tmp_f - 2) + 1);
      }
      if(sym_ == sym) {
        dach_set_freq(arc, sym_, (tmp_f + 2) + 1);
      }
    }
    */
    //sum += dach_get_freq(arc, sym_);
    //h_set_cumfreq(arc, sym_, sum);
    dach_inc_cumfreq(ppm, sym_);
  }
  ppm->sym_prev = sym;
  sum = dach_ppm0_cumfreq(ppm, 1);
  ppm->scale = sum;
  //fprintf(stderr, "h_ppm0_cumfreq(ppm, 0) ==%llu\n", dach_ppm0_cumfreq(arc, 0));
  assert(dach_ppm0_cumfreq(ppm, 0) == 0);
  assert(ppm->scale == dach_ppm0_cumfreq(ppm, 1));
  return ppm->scale;
}

/*
static void dach_write_stream(PPM0 *ppm, u32 bit) {
  //fprintf(stderr, "h_write_stream(%u)\n", bit);
  assert(bit == 1 || bit == 0);
  dach_bitout(arc, bit);
  while(ppm->underflow > 0) {
    dach_bitout(arc, ! bit);   
    ppm->underflow--;
  }
} 


static int dach_bitout(PPM0 *ppm, u8 bit) {
  int rv = 0;
  //fprintf(stderr, "h_bitout(%u) total_bits=%llu\n", bit, ppm->total_bits);
  if(ppm->io_buffer_full == 0) {
    ppm->io_buffer_full = ppm->io_buffer_size;
    int st = fwrite(&ppm->io_buffer, sizeof(u8), 4, ppm->out);
    //fprintf(stderr, "writing integer = %u\n", ppm->io_buffer);
    //fflush(ppm->out);
    ppm->io_buffer = 0;
  }
  ppm->total_bits_out++;
  ppm->io_buffer <<= 1;
  ppm->io_buffer += bit;
  ppm->io_buffer_full--;
  return rv;
}
*/

/*
static void dach_print_symtab(const PPM0 *ppm, const char *file, const int line) {
  u32 sym = 0;
  for(sym = 0 ; sym <= DACH_SYM_COUNT; sym++) {
    //fprintf(  stderr, "sym=%u\n",sym);
    u32 chr = ppm->sym_to_chr[sym];
    u64 f   = ppm->freq[chr];
    u64 cf  = ppm->cumfreq[chr];
    fprintf(stderr, "chr=%c sym=%u f=%llu cumfreq=%llu\n", chr, sym, f, cf);
  }
}
*/

/*
static u32 dach_symbin = 16;
static long long int dach_loop_savings = 0;
static u32 dach_find_sym_bin(PPM0 *ppm) {
  ppm->range = dach_calc_range(arc);
  u64 cumfreq = (((u64)(ppm->esym - ppm->low) + 1UL)*ppm->scale - 1)/ppm->range;
  u32 sym  = 0;
  u32 sym_bin = 0;
  u32 loop_bin_real = 0;
  u64 cf = dach_ppm0_cumfreq(arc, dach_symbin);
  if(cf > cumfreq) {
    while(++h_symbin < DACH_SYM_COUNT && dach_ppm0_cumfreq(arc, dach_symbin) > cumfreq) {
      loop_bin_real++;
    };
    dach_symbin--;
  }
  else {
    while( --h_symbin < DACH_SYM_COUNT && dach_ppm0_cumfreq(arc, dach_symbin) <= cumfreq) {
      loop_bin_real++;
    };
  }
  sym = dach_symbin;
  find_bin += loop_bin_real;
  times_bin++;
  if(find_bin > 100000) {
     dach_symbin = find_bin/times_bin/2 + 1;
     //fprintf(stderr, "loop_bin_real=%u loop=%u dach_symbin=%u\n", loop_bin_real, dach_symbin);
     times_bin = 0;
     find_bin  = 0;
  }
  //exit(0);
  if(sym == 0) {
    ppm->sym = sym;
    DACH_M_PRINT_SYMTAB(arc);
    fprintf(stderr, "cfreq=%llu sym=0\n", cumfreq);
    fprintf(stderr, "h_ppm0_cumfreq==%llu, cfreq=%llu, sym=%u\n", dach_ppm0_cumfreq(arc, sym), cumfreq, sym);
    assert(0);
  }
  return sym;
}
*/



/*
    if(((tmp = fgetc(ppm->in)) == EOF)) {
      fprintf(stderr, "eof\n");
      ppm->eof_in = 1;
    }
    if(tmp < 256) {
      u8 v = tmp; // input bits to be reversed
      u8 r = v;   // r will be reversed bits of v; first get LSB of v
      int s = 7;  // extra shift needed at end

      for (v >>= 1; v; v >>= 1)
      {   
        r <<= 1;
        r |= v & 1;
        s--;
      }
      r <<= s; // shift when v's highest bits are zero
      ppm->io_buffer = (u8)r;
    }
    else {
      ppm->io_buffer = 0;
    }
*/


    //fprintf(stderr, "loop=%i int=%u\n", loop, ppm->esym);
    /*
    if( ppm->high < dach_MSB ) {
      //fprintf(stderr, "0|\n");
    }
    else if( ppm->low >= dach_MSB ) {
      //fprintf(stderr, "1|\n");
      ppm->esym -= dach_MSB;
      ppm->high -= dach_MSB; 
      ppm->low  -= dach_MSB; 
      //fprintf(stderr, "loop=%i dach_MSB=%u int=%u\n", loop, dach_MSB,  ppm->esym);
    }
    else if((ppm->low >= dach_1QTR) && (ppm->high < dach_3QTR)) {
      //fprintf(stderr, "2|\n");
      ppm->esym -= dach_1QTR;
      ppm->high -= dach_1QTR;
      ppm->low  -= dach_1QTR;
    }
    else {
      break;
    }
    */
    /*
    if( ppm->high < dach_MSB ) {
      //fprintf(stderr, "0|");
      dach_write_stream(arc, (u8)0);
    }
    else if( ppm->low >= dach_MSB ) {
      //fprintf(stderr, "1|");
      dach_write_stream(arc, (u8)1);
      ppm->high -= dach_MSB; 
      ppm->low  -= dach_MSB; 
    }
    else if((ppm->low >= dach_1QTR) && (ppm->high < dach_3QTR)) {
      ppm->underflow++;
      ppm->high -= dach_1QTR;
      ppm->low  -= dach_1QTR;
    }
    else {
      break;
    }
    */


  /*
  if(sym != '1' && sym != '\n') {
    fprintf(stderr, "sym=>%c<%u>\n", sym, sym);
    DACH_M_PRINT_SYMTAB(arc);
    if(err++ > 3) { 
      exit(0);
    }
  }
  if(ppm->total_syms > 3295174) {
    fprintf(stderr, "sym=>%c<%u>\n", sym, sym);
    DACH_M_PRINT_SYMTAB(arc);
    if(ppm->total_syms > 3295250) {
      exit(0);
    }
  }
  */

  /*
  if(ppm->total_syms > 3295154) {
    fprintf(stderr, "sym=>%c<%u>\n", sym, sym);
    DACH_M_PRINT_SYMTAB(arc);
    if(ppm->total_syms > 3295254) {
      exit(0);
    }
  }
  if(ppm->total_syms > 4000) {
    fprintf(stderr, "sym=>%c<%u>\n", sym, sym);
    DACH_M_PRINT_SYMTAB(arc);
    if(ppm->total_syms > 4005) {
      exit(0);
    }
  }
  */


  /*
  if(ppm->scale >= 0xf3ffff) {
    assert(0);
    for(sym_ = 0; sym_ < DACH_SYM_COUNT; sym_++) {
      freq = dach_get_freq(arc, sym_);
      if(freq > 0xF) {
        freq = freq/8 + 1;
        dach_set_freq(arc, sym_, freq);
        sum += freq;
        dach_set_cumfreq(arc, sym_, sum);
      }
      else {
        dach_set_freq(arc, sym_, 1);
        sum += 1;
        dach_set_cumfreq(arc, sym_, sum);
      }
    }
  }
  */
/*
static int dach_test_freq(PPM0 *ppm, const u32 sym) {
  u32 chr = dach_sym_to_chr(arc, sym);
  if(ppm->symtable[chr] != ppm->freq[chr]) {
    //fprintf(stderr, "ppm->symtable[%c]==%u != ppm->freq[%c]==%u\n", chr, ppm->symtable[chr], chr, ppm->freq[chr]);
    //assert(0);
  }
/  return 1;
}
*/
/*
static u32 sym_to_char(const u32 sym) {
  assert(sym >= DACH_SYM_START);
  return sym - DACH_SYM_START;
}
*/




#undef OBJ
#undef DACH_M_CHECK_BIT
#undef DACH_M_PRINT_SYMTAB

