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


HistoryNode *new_history_node(SimulationHistory *simulation_history, int seed) {
    HistoryNode *history_node = malloc(sizeof(HistoryNode));
    history_node->seed = seed;
    history_node->simulation_history = simulation_history;
    history_node->next = NULL;
    return history_node;
}


void free_history_node(HistoryNode *history_node) {
    free(history_node);
}

HistoryQueue *new_history_queue() {
    HistoryQueue *history_queue = malloc(sizeof(HistoryQueue));
    pthread_mutex_init(&history_queue->mtx, NULL);
    history_queue->history_node = NULL;
    return history_queue;
}

void free_history_queue(HistoryQueue *history_queue) {
    pthread_mutex_destroy(&history_queue->mtx);
    free(history_queue);
}

void insert_simulation_history(
    HistoryQueue *history_queue,
    SimulationHistory *simulation_history,
    int seed
    ) {

    pthread_mutex_lock(&history_queue->mtx);
    HistoryNode *history_node = new_history_node(simulation_history, seed);
    history_node->next = history_queue->history_node;
    history_queue->history_node = history_node;
    pthread_mutex_unlock(&history_queue->mtx);
}

int get_simulation_history(
    HistoryQueue *history_queue,
    SimulationHistory **simulation_history) {

    int seed = - 1;
    pthread_mutex_lock(&history_queue->mtx);
    HistoryNode *history_node = history_queue->history_node;


    if (history_node) {
        history_queue->history_node = history_node->next;
        *simulation_history = history_node->simulation_history;
        seed = history_node->seed;
        free_history_node(history_node);
    }

    pthread_mutex_unlock(&history_queue->mtx);
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
    dp->hq = new_history_queue();
    dp->sq = new_seed_queue(number_of_simulations, base_seed);
    dp->number_of_threads = number_of_threads;
    dp->threads = malloc(sizeof(pthread_t) * dp->number_of_threads);
    dp->logging = logging;
    dp->step_cutoff = step_cutoff;

    return dp;
}

void free_dispatcher(Dispatcher *dispatcher) {
    sqlite3_close(dispatcher->db);
    free_reaction_network(dispatcher->rn);
    free_history_queue(dispatcher->hq);
    free_seed_queue(dispatcher->sq);
    free(dispatcher->threads);
    free(dispatcher);
}


void run_dispatcher(Dispatcher *dispatcher) {
  int i;
  SimulatorPayload *simulation;

  for (i = 0; i < dispatcher->number_of_threads; i++) {
    simulation = new_simulator_payload(
        dispatcher->rn,
        dispatcher->hq,
        tree,
        dispatcher->sq,
        dispatcher->step_cutoff);

    pthread_create(
        dispatcher->threads + i,
        NULL,
        run_simulator,
        (void *)simulation);
  }


  for (i = 0; i < dispatcher->number_of_threads; i++) {
    pthread_join(dispatcher->threads[i],NULL);
  }


  SimulationHistory *simulation_history = NULL;

  int seed = get_simulation_history(dispatcher->hq, &simulation_history);

  while (seed != -1) {
      free_simulation_history(simulation_history);
      seed = get_simulation_history(dispatcher->hq, &simulation_history);
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

        insert_simulation_history(sp->hq, simulation->history, seed);
        free_simulation(simulation);
        seed = get_seed(sp->sq);
    }

    free_simulator_payload(sp);
    pthread_exit(NULL);
}
