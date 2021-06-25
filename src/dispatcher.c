#include "dispatcher.h"

SeedQueue *new_seed_queue(int number_of_seeds, unsigned int base_seed) {

  SeedQueue *sqp = malloc(sizeof(SeedQueue));
  unsigned int *seeds = malloc(sizeof(unsigned int) * number_of_seeds);

  for (unsigned int i = 0; i < number_of_seeds; i++) {
      seeds[i] = base_seed + i;
  }

  sqp->seeds = seeds;
  sqp->number_of_seeds = number_of_seeds;
  sqp->next_seed = 0;
  pthread_mutex_init(&sqp->mtx,NULL);
  return sqp;
}

void free_seed_queue(SeedQueue *sqp) {
  free(sqp->seeds);
  pthread_mutex_destroy(&sqp->mtx);
  free(sqp);
}

unsigned long int get_seed(SeedQueue *sqp) {
  pthread_mutex_lock(&sqp->mtx);
  unsigned long int seed;
  if (sqp->next_seed < sqp->number_of_seeds) {
    seed = sqp->seeds[sqp->next_seed];
    sqp->next_seed++;
  }
  else
    seed = 0;

  pthread_mutex_unlock(&sqp->mtx);
  return seed;
}


Dispatcher *new_dispatcher(
    char *database_file,
    int number_of_simulations,
    int base_seed,
    int number_of_threads,
    int step_cutoff,
    bool logging) {


    Dispatcher *dp = malloc(sizeof(Dispatcher));
    sqlite3_open(database_file, &dp->db);
    dp->rn = new_reaction_network(dp->db);
    dp->sq = new_seed_queue(number_of_simulations, base_seed);
    dp->number_of_threads = number_of_threads;
    dp->threads = malloc(sizeof(pthread_t) * dp->number_of_threads);
    dp->logging = logging;
    dp->step_cutoff = step_cutoff;

    return dp;
}

void free_dispatcher(Dispatcher *dp) {
    sqlite3_close(dp->db);
    free_reaction_network(dp->rn);
    free_seed_queue(dp->sq);
    free(dp->threads);
    free(dp);
}
