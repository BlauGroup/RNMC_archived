#include "dispatcher.h"

char *number_of_seeds_postfix = "/number_of_seeds";
char *number_of_threads_postfix = "/number_of_threads";
char *seeds_postfix = "/seeds";
char *time_cutoff_postfix = "/time_cutoff";
char *step_cutoff_postfix = "/step_cutoff";


SeedQueue *new_seed_queue(int number_of_seeds, unsigned long int *seeds) {
  SeedQueue *sqp = malloc(sizeof(SeedQueue));
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




Dispatcher *new_dispatcher(char *reaction_network_dir,
                           char *simulation_params,
                           bool logging) {

  char path[2048];
  FILE *file;
  int return_code;
  int i;
  char *end;

  Dispatcher *dp = malloc(sizeof(Dispatcher));
  dp->reaction_network_dir = reaction_network_dir;
  dp->logging = logging;
  dp->rn = new_reaction_network(reaction_network_dir, logging);
  if (!dp->rn) {
    puts("reaction network wasn't created");
    return NULL;
  }

  dp->simulation_params = simulation_params;

  // read number_of_seeds
  int number_of_seeds;
  end = stpcpy(path, simulation_params);
  stpcpy(end, number_of_seeds_postfix);
  file = fopen(path,"r");
  if (!file) {
    printf("new_dispatcher: cannot open %s\n",path);
    return NULL;
  } else {
    return_code = fscanf(file, "%d\n", &number_of_seeds);
    fclose(file);
  }

  // read seeds
  unsigned long int *seeds = malloc(sizeof(unsigned long int) * number_of_seeds);
  end = stpcpy(path, simulation_params);
  stpcpy(end, seeds_postfix);
  file = fopen(path,"r");
  if (!file) {
    printf("new_dispatcher: cannot open %s\n",path);
    return NULL;
  } else {
    for (i = 0; i < number_of_seeds; i++) {
      return_code = fscanf(file, "%lu\n", seeds + i);
    }
    fclose(file);
  }

  dp->sq = new_seed_queue(number_of_seeds, seeds);

  // read number_of_threads
  end = stpcpy(path, simulation_params);
  stpcpy(end, number_of_threads_postfix);
  file = fopen(path,"r");
  if (!file) {
    printf("new_dispatcher: cannot open %s\n", path);
    return NULL;
  } else {
    return_code = fscanf(file, "%d\n", &dp->number_of_threads);
    fclose(file);
  }

  // read step_cutoff
  end = stpcpy(path, simulation_params);
  stpcpy(end, step_cutoff_postfix);
  file = fopen(path,"r");
  if (!file) {

    if (logging) {
      printf("new_dispatcher: cannot open %s.\n",
             path);
    }
    // read time_cutoff
    end = stpcpy(path, simulation_params);
    stpcpy(end, time_cutoff_postfix);
    file = fopen(path,"r");
    if (!file) {
      printf("new_dispatcher: cannot open %s\n",path);
      return NULL;
    } else {
      return_code = fscanf(file, "%lf\n", &dp->time_cutoff);
      dp->step_cutoff = -1;
      if (logging)
        puts("using time cutoff");
      dp->cutoff_type = time_cutoff;
      fclose(file);
    }
  }

  else {
    return_code = fscanf(file, "%d\n", &dp->step_cutoff);
    if (logging)
      puts("using step cutoff");
    dp->cutoff_type = step_cutoff;
    dp->time_cutoff = 0;
    fclose(file);
  }



  dp->threads = malloc(sizeof(pthread_t) * dp->number_of_threads);
  return dp;
}

void free_dispatcher(Dispatcher *dp) {
  free_reaction_network(dp->rn);
  free_seed_queue(dp->sq);
  free(dp->threads);
  free(dp);
}

void run_dispatcher(Dispatcher *dp) {
  int i;
  SimulatorPayload *sp;
  if (dp->logging)
    printf("spawning %d threads\n",dp->number_of_threads);

  for (i = 0; i < dp->number_of_threads; i++) {
    sp = new_simulator_payload(dp->rn,
                               tree,
                               dp->sq,
                               dp->cutoff_type,
                               dp->time_cutoff,
                               dp->step_cutoff);
    pthread_create(dp->threads + i, NULL, run_simulator, (void *)sp);
  }

  if (dp->logging)
    puts("dispatcher waiting for threads to terminate");

  for (i = 0; i < dp->number_of_threads; i++) {
    pthread_join(dp->threads[i],NULL);
  }


}

SimulatorPayload *new_simulator_payload(ReactionNetwork *rn,
                                        SolveType type,
                                        SeedQueue *sq,
                                        CutoffType cutoff_type,
                                        double time_cutoff,
                                        int step_cutoff
                                        ) {

  SimulatorPayload *spp = malloc(sizeof(SimulatorPayload));
  spp->rn = rn;
  spp->type = type;
  spp->sq = sq;
  spp->cutoff_type = cutoff_type;
  spp->time_cutoff = time_cutoff;
  spp->step_cutoff = step_cutoff;
  return spp;
}

void free_simulator_payload(SimulatorPayload *sp) {
  // reaction network and seed queue get freed as part of the dispatcher
  free(sp);
}

void *run_simulator(void *simulator_payload) {
  SimulatorPayload *sp = (SimulatorPayload *) simulator_payload;
  unsigned long int seed = get_seed(sp->sq);
  while (seed > 0) {
    Simulation *simulation = new_simulation(sp->rn, seed, sp->type);

    switch (sp->cutoff_type) {
      case time_cutoff:
        run_until(simulation, sp->time_cutoff);
        break;
      case step_cutoff:
        run_for(simulation, sp->step_cutoff);
        break;
    }
    simulation_history_to_file(simulation);

    // this is nice for small batches, but unwieldy for large batches
    // if (sp->rn->logging)
    //  printf("simulation %ld finished: %d steps\n",seed, simulation->step);
    free_simulation(simulation);
    seed = get_seed(sp->sq);
  }

  free_simulator_payload(sp);
  pthread_exit(NULL);
}
