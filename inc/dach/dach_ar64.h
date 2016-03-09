#ifndef DACH_AR64_H
#define DACH_AR64_H
#include "dach_ppm0.h"
#include "dach_stream.h"

#define DACH_EOF 0xDEC0DED
typedef struct DACH_AR_CODER ARC;
ARC *dach_new_arcoder_(dach_ppm_model *mod, dach_block *block);
ARC *dach_new_arcoder(dach_ppm_model *mod, FILE *in, FILE *out, u32 symcount, u64 size);
void dach_delete_arcoder(ARC *arc);

int dach_stop_encoder(ARC *arc);
int dach_start_decoder(ARC *arc);
int dach_decode(ARC *arc);
int dach_encode(ARC *arc, u32 sym);
int dach_eof(ARC *arc);

//ARC * dach_new_arcoder(FILE *in, FILE *out, u32 symcount, u64 size);
//ARC * dach_new_arcoder(PPM0 *ppm, FILE *in, FILE *out, u32 symcount, u64 size);
/* TEST GETTERS, remove when not testing */
/*
u32 dach_get_arc_high(ARC *arc);
u32 dach_get_arc_low(ARC *arc);
u32 dach_get_arc_range(ARC *arc);
u32 dach_get_arc_scale(ARC *arc);
u32 dach_get_arc_scale(ARC *arc);
u32 dach_get_arc_underflow(ARC *arc);
u64 dach_get_calc_range(ARC *arc);
u32 dach_get_high_scale(const ARC *arc, const u8 sym);
u32 dach_get_low_scale(const ARC *arc, const u32 sym);
u64 dach_do_get_cumfreq(const ARC *arc, const u32 sym);
u32  dach_get_arc_io_buffer(ARC *arc);
u32  dach_get_arc_io_buffer_full(ARC *arc);
u32  dach_get_arc_io_buffer_size(ARC *arc);
u32 dach_do_find_sym(ARC *arc);
void dach_do_encode_step(ARC *arc, const u8 sym);
void dach_do_write_stream(ARC *arc, const u8 bit);
void dach_do_print_symtab(const ARC *arc);
*/

#endif /* DACH_AR64_H */
