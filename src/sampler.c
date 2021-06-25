#include "sampler.h"

Sampler *new_sampler(unsigned long int seed) {

  Sampler *p = malloc(sizeof(Sampler));


  p->internal_rng_state = gsl_rng_alloc(gsl_rng_default);
  p->seed = seed;

  gsl_rng_set(p->internal_rng_state, seed);

  p->generate = &generate_method;

  return p;
}

void free_sampler(Sampler *p) {
  gsl_rng_free(p->internal_rng_state);
  free(p);
}

double generate_method(void *samplerp) {
  Sampler *p = (Sampler *) samplerp;
  return gsl_rng_uniform_pos(p->internal_rng_state);
}
