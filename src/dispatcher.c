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


HistoryNode *new_history_node(SimulationHistory *simulation_history) {
    HistoryNode *p = malloc(sizeof(HistoryNode));
    p->simulation_history = simulation_history;
    p->next = NULL;
    return p;
}

void free_history_node(HistoryNode *hnp) {
    free(hnp);
}

HistoryQueue *new_history_queue() {
    HistoryQueue *hqp = malloc(sizeof(HistoryQueue));
    pthread_mutex_init(&hqp->mtx, NULL);
    hqp->history_node = NULL;
    return hqp;
}

void free_history_queue(HistoryQueue *hqp) {
    pthread_mutex_destroy(&hqp->mtx);
    free(hqp);
}

void insert_simulation_history(
    HistoryQueue *hqp,
    SimulationHistory *shp) {

    pthread_mutex_lock(&hqp->mtx);
    HistoryNode *hnp = new_history_node(shp);
    hnp->next = hqp->history_node;
    hqp->history_node = hnp;
    pthread_mutex_unlock(&hqp->mtx);
}

SimulationHistory *get_simulation_history(HistoryQueue *hqp) {


    pthread_mutex_lock(&hqp->mtx);
    SimulationHistory *result = NULL;
    HistoryNode *node = hqp->history_node;


    if (node) {
        hqp->history_node = node->next;
        result = node->simulation_history;
        free_history_node(node);
    }

    pthread_mutex_unlock(&hqp->mtx);
    return result;

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
    dp->hq = new_history_queue();
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
    free_history_queue(dp->hq);
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
    sp = new_simulator_payload(
        dp->rn,
        dp->hq,
        tree,
        dp->sq,
        dp->step_cutoff);

    pthread_create(dp->threads + i, NULL, run_simulator, (void *)sp);
  }

  if (dp->logging)
    puts("dispatcher waiting for threads to terminate");

  for (i = 0; i < dp->number_of_threads; i++) {
    pthread_join(dp->threads[i],NULL);
  }


  SimulationHistory *shp = get_simulation_history(dp->hq);
  while (shp) {
      free_simulation_history(shp);
      shp = get_simulation_history(dp->hq);
  }

}

SimulatorPayload *new_simulator_payload(
    ReactionNetwork *rn,
    HistoryQueue *hq,
    SolveType type,
    SeedQueue *sq,
    int step_cutoff
    ) {

    SimulatorPayload *spp = malloc(sizeof(SimulatorPayload));
    spp->rn = rn;
    spp->hq = hq;
    spp->type = type;
    spp->sq = sq;
    spp->step_cutoff = step_cutoff;
    return spp;
}

void free_simulator_payload(SimulatorPayload *sp) {
    // reaction network, seed queue and history queue
    // get freed as part of the dispatcher
    free(sp);
}

void *run_simulator(void *simulator_payload) {
    SimulatorPayload *sp = (SimulatorPayload *) simulator_payload;
    unsigned long int seed = get_seed(sp->sq);
    while (seed > 0) {
        Simulation *simulation = new_simulation(sp->rn, seed, sp->type);

        run_for(simulation, sp->step_cutoff);

        insert_simulation_history(sp->hq, simulation->history);
        free_simulation(simulation);
        seed = get_seed(sp->sq);
    }

    free_simulator_payload(sp);
    pthread_exit(NULL);
}
