#ifndef SAMPLER_H
#define SAMPLER_H
#include <gsl/gsl_rng.h>
#include <stdlib.h>


typedef struct sampler {
  gsl_rng *internal_rng_state;
  unsigned long int seed;
  double (*generate)(void *samplerp);
} Sampler;


Sampler *new_sampler(unsigned long int seed);
void free_sampler(Sampler *p);
double generate_method(void *samplerp);

#endif
