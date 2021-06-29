#include "dispatcher.h"

SeedQueue *new_seed_queue(int number_of_seeds, unsigned int base_seed) {

    SeedQueue *seed_queue = calloc(1,sizeof(SeedQueue));
    unsigned int *seeds = calloc(number_of_seeds, sizeof(unsigned int));

  for (unsigned int i = 0; i < number_of_seeds; i++) {
      seeds[i] = base_seed + i;
  }

  seed_queue->seeds = seeds;
  seed_queue->number_of_seeds = number_of_seeds;
  seed_queue->next_seed = 0;
  pthread_mutex_init(&seed_queue->mutex,NULL);
  return seed_queue;
}

void free_seed_queue(SeedQueue *seed_queue) {
  free(seed_queue->seeds);
  pthread_mutex_destroy(&seed_queue->mutex);
  free(seed_queue);
}

unsigned long int get_seed(SeedQueue *seed_queue) {
  pthread_mutex_lock(&seed_queue->mutex);
  unsigned long int seed;
  if (seed_queue->next_seed < seed_queue->number_of_seeds) {
    seed = seed_queue->seeds[seed_queue->next_seed];
    seed_queue->next_seed++;
  }
  else
    seed = 0;

  pthread_mutex_unlock(&seed_queue->mutex);
  return seed;
}


HistoryNode *new_history_node(SimulationHistory *simulation_history, int seed) {
    HistoryNode *history_node = calloc(1,sizeof(HistoryNode));
    history_node->seed = seed;
    history_node->simulation_history = simulation_history;
    history_node->next = NULL;
    return history_node;
}


void free_history_node(HistoryNode *history_node) {
    free(history_node);
}

HistoryQueue *new_history_queue() {
    HistoryQueue *history_queue = calloc(1,sizeof(HistoryQueue));
    pthread_mutex_init(&history_queue->mutex, NULL);
    history_queue->history_node = NULL;
    return history_queue;
}

void free_history_queue(HistoryQueue *history_queue) {
    pthread_mutex_destroy(&history_queue->mutex);
    free(history_queue);
}

void insert_simulation_history(
    HistoryQueue *history_queue,
    SimulationHistory *simulation_history,
    int seed
    ) {

    pthread_mutex_lock(&history_queue->mutex);
    HistoryNode *history_node = new_history_node(simulation_history, seed);
    history_node->next = history_queue->history_node;
    history_queue->history_node = history_node;
    pthread_mutex_unlock(&history_queue->mutex);
}

int get_simulation_history(
    HistoryQueue *history_queue,
    SimulationHistory **simulation_history) {

    pthread_mutex_lock(&history_queue->mutex);
    int seed = - 1;
    HistoryNode *history_node = history_queue->history_node;


    if (history_node) {
        history_queue->history_node = history_node->next;
        *simulation_history = history_node->simulation_history;
        seed = history_node->seed;
        free_history_node(history_node);
    }

    pthread_mutex_unlock(&history_queue->mutex);
    return seed;

}

char sql_insert_trajectory[] =
    "INSERT INTO trajectories VALUES (?1, ?2, ?3, ?4);";

Dispatcher *new_dispatcher(
    char *database_file,
    int number_of_simulations,
    int base_seed,
    int number_of_threads,
    int step_cutoff,
    bool logging) {


    Dispatcher *dispatcher = calloc(1,sizeof(Dispatcher));
    sqlite3_open(database_file, &dispatcher->database);

    int rc = sqlite3_prepare_v2(
        dispatcher->database,
        sql_insert_trajectory,
        -1,
        &dispatcher->insert_trajectory_stmt,
        NULL);

    if (rc != SQLITE_OK) {
        printf("new_dispatcher error %s\n", sqlite3_errmsg(dispatcher->database));
        return NULL;
    }

    dispatcher->reaction_network = new_reaction_network(dispatcher->database);
    dispatcher->history_queue = new_history_queue();
    dispatcher->seed_queue = new_seed_queue(number_of_simulations, base_seed);
    dispatcher->number_of_threads = number_of_threads;
    dispatcher->running = calloc(number_of_threads, sizeof(bool));

    dispatcher->threads = calloc(
        dispatcher->number_of_threads,
        sizeof(pthread_t)
        );

    dispatcher->logging = logging;
    dispatcher->step_cutoff = step_cutoff;

    return dispatcher;
}

void free_dispatcher(Dispatcher *dispatcher) {
    sqlite3_finalize(dispatcher->insert_trajectory_stmt);
    sqlite3_close(dispatcher->database);
    free_reaction_network(dispatcher->reaction_network);
    free_history_queue(dispatcher->history_queue);
    free_seed_queue(dispatcher->seed_queue);
    free(dispatcher->threads);
    free(dispatcher->running);
    free(dispatcher);
}

void dispatcher_log(Dispatcher *dispatcher, int seed) {
    if (dispatcher->logging)
    {
        time_t rawtime;
        struct tm * timeinfo;

        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        printf(
            "[%d:%d:%d] writing trajectory %d to database\n",
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec,
            seed);
    }
}

void run_dispatcher(Dispatcher *dispatcher) {
    int i;
    SimulatorPayload *simulation;
    SimulationHistory *simulation_history = NULL;
    bool flag;


    for (i = 0; i < dispatcher->number_of_threads; i++) {
        simulation = new_simulator_payload(
            dispatcher->reaction_network,
            dispatcher->history_queue,
            tree,
            dispatcher->seed_queue,
            dispatcher->step_cutoff);

        pthread_create(
            dispatcher->threads + i,
            NULL,
            run_simulator,
            (void *)simulation);

        dispatcher->running[i] = true;
    }


    while (true) {

        int seed = get_simulation_history(
            dispatcher->history_queue,
            &simulation_history);

        while (seed != -1) {

            dispatcher_log(dispatcher, seed);

            record_simulation_history(
                dispatcher,
                simulation_history, seed);

            seed = get_simulation_history(
                dispatcher->history_queue,
                &simulation_history);
        }

        for (i = 0; i < dispatcher->number_of_threads; i++) {
            if (dispatcher->running[i])
                if (pthread_tryjoin_np(dispatcher->threads[i],NULL) == 0)
                    dispatcher->running[i] = false;
        }

        flag = false;
        for (i = 0; i < dispatcher->number_of_threads; i++) {
            flag = flag || dispatcher->running[i];
        }

        if (! flag)
            break;


    }
}

void record_simulation_history(
    Dispatcher *dispatcher,
    SimulationHistory *simulation_history,
    int seed
    ) {

    int count = 0;
    int i;
    int rc;
    Chunk *chunk = simulation_history->first_chunk;

    sqlite3_exec(dispatcher->database, "BEGIN", 0, 0, 0);

    while (chunk) {
        for (i = 0; i < chunk->next_free_index; i++) {


            sqlite3_bind_int(dispatcher->insert_trajectory_stmt, 1, seed);
            sqlite3_bind_int(dispatcher->insert_trajectory_stmt, 2, count);

            sqlite3_bind_int(dispatcher->insert_trajectory_stmt, 3,
                             chunk->data[i].reaction);

            sqlite3_bind_double(dispatcher->insert_trajectory_stmt, 4,
                             chunk->data[i].time);

            rc = sqlite3_step(dispatcher->insert_trajectory_stmt);
            sqlite3_reset(dispatcher->insert_trajectory_stmt);
            count += 1;

            if (count % TRANSACTION_SIZE == 0) {
                sqlite3_exec(dispatcher->database, "COMMIT", 0, 0, 0);
                sqlite3_exec(dispatcher->database, "BEGIN", 0, 0, 0);
            }

        }
        chunk = chunk->next_chunk;
    }

    sqlite3_exec(dispatcher->database, "COMMIT", 0, 0, 0);

    // free simulation history once we have inserted it into the db
    free_simulation_history(simulation_history);
}



SimulatorPayload *new_simulator_payload(
    ReactionNetwork *reaction_network,
    HistoryQueue *history_queue,
    SolveType type,
    SeedQueue *seed_queue,
    int step_cutoff
    ) {

    SimulatorPayload *spp = calloc(1, sizeof(SimulatorPayload));
    spp->reaction_network = reaction_network;
    spp->history_queue = history_queue;
    spp->type = type;
    spp->seed_queue = seed_queue;
    spp->step_cutoff = step_cutoff;
    return spp;
}

void free_simulator_payload(SimulatorPayload *sp) {
    // reaction network, seed queue and history queue
    // get freed as part of the dispatcher
    free(sp);
}

void *run_simulator(void *sp) {
    SimulatorPayload *simulator_payload = (SimulatorPayload *) sp;
    unsigned long int seed = get_seed(simulator_payload->seed_queue);
    while (seed > 0) {
        Simulation *simulation = new_simulation(
            simulator_payload->reaction_network,
            seed,
            simulator_payload->type);

        run_for(simulation, simulator_payload->step_cutoff);

        insert_simulation_history(
            simulator_payload->history_queue,
            simulation->history,
            seed);


        free_simulation(simulation);
        seed = get_seed(simulator_payload->seed_queue);
    }

    free_simulator_payload(sp);
    pthread_exit(NULL);
}
