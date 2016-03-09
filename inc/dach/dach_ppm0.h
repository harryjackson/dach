#ifndef DACH_PPM0_H
#define DACH_PPM0_H
#include "dach_types.h"

typedef struct dach_ppm_model dach_ppm_model;

typedef struct dach_ppm_api {
  u32 (*find_sym)(dach_ppm_model *ppm, u64 cumfreq);
  u64 (*update)(dach_ppm_model *ppm  , const u32 sym);
  u64 (*scale)(dach_ppm_model *ppm);
  u64 (*cumfreq)(const dach_ppm_model *ppm, const u32 sym);
  u32 (*chr_to_sym)(dach_ppm_model *ppm   , const u32 chr);
  u32 (*sym_to_chr)(dach_ppm_model *ppm , const u32 sym);
} dach_ppm_api;

struct dach_ppm_model {
  dach_ppm_api *api;
  void *priv;
};

dach_ppm_model *dach_ppm0_new_(void);
void            dach_ppm0_free_(dach_ppm_model *mod);


/*
//typedef struct dach_MODEL_PPM0 PPM0;
//PPM0 * dach_new_ppm0(FILE *in, FILE *out, u32 symcount, u64 size);
PPM0 *h_ppm0_new(void);
void  dach_ppm0_free(PPM0 *ppm);
*/

#endif /* DACH_PPM0_H */
