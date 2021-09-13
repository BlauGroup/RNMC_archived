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

char sql_remove_duplicate_trajectories[] =
    "DELETE FROM trajectories WHERE rowid NOT IN"
    "(SELECT MIN(rowid) FROM trajectories GROUP BY seed, step);";


Dispatcher *new_dispatcher(
    char *reaction_database_file,
    char *initial_state_database_file,
    int number_of_simulations,
    int base_seed,
    int number_of_threads,
    int step_cutoff,
    long int gc_interval,
    int gc_threshold,
    bool logging) {


    Dispatcher *dispatcher = calloc(1,sizeof(Dispatcher));
    sqlite3_open(reaction_database_file, &dispatcher->reaction_database);
    sqlite3_open(initial_state_database_file, &dispatcher->initial_state_database);

    int rc = sqlite3_prepare_v2(
        dispatcher->initial_state_database,
        sql_insert_trajectory,
        -1,
        &dispatcher->insert_trajectory_stmt,
        NULL);

    if (rc != SQLITE_OK) {
        printf("new_dispatcher error %s\n", sqlite3_errmsg(
                   dispatcher->initial_state_database));
        return NULL;
    }

    dispatcher->reaction_network = new_reaction_network(
        dispatcher->reaction_database,
        dispatcher->initial_state_database
        );

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
    dispatcher->start_time = time(NULL);
    dispatcher->gc_interval = gc_interval;
    dispatcher->gc_threshold = gc_threshold;

    return dispatcher;
}

void free_dispatcher(Dispatcher *dispatcher) {
    sqlite3_finalize(dispatcher->insert_trajectory_stmt);
    sqlite3_close(dispatcher->reaction_database);
    sqlite3_close(dispatcher->initial_state_database);
    free_reaction_network(dispatcher->reaction_network);
    free_history_queue(dispatcher->history_queue);
    free_seed_queue(dispatcher->seed_queue);
    free(dispatcher->threads);
    free(dispatcher->running);
    free(dispatcher);
}

void dispatcher_log(Dispatcher *dispatcher, char *message) {
    if (dispatcher->logging)
    {
        time_t rawtime;
        struct tm * timeinfo;

        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        printf(
            "[%02d:%02d:%02d] %s",
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec,
            message);
    }
}



void run_dispatcher(Dispatcher *dispatcher) {
    int i;
    SimulatorPayload *simulation;
    SimulationHistory *simulation_history = NULL;
    bool flag;
    char log_buffer[256];
    long int time_since_start;
    int seed;


    for (i = 0; i < dispatcher->number_of_threads; i++) {
        simulation = new_simulator_payload(
            dispatcher->reaction_network,
            dispatcher->history_queue,
            tree,
            dispatcher->seed_queue,
            dispatcher->step_cutoff,
            dispatcher->running + i
            );

        pthread_create(
            dispatcher->threads + i,
            NULL,
            run_simulator,
            (void *)simulation);

        dispatcher->running[i] = true;
    }


    long int previous_time_since_start = time(0) - dispatcher->start_time;

    while (true) {

        // garbage collection
        time_since_start = time(0) - dispatcher->start_time;

        if ( time_since_start - previous_time_since_start >
             dispatcher->gc_interval ) {

            previous_time_since_start = time_since_start;

            int number_of_nodes_freed =
                garbage_collect_dependency_graph(
                    dispatcher->reaction_network,
                    dispatcher->gc_threshold
                    );

            sprintf(
                log_buffer,
                "garbage collected %d nodes\n",
                number_of_nodes_freed);

            dispatcher_log(dispatcher, log_buffer);


        }

        // recording trajectories
        seed = get_simulation_history(
            dispatcher->history_queue,
            &simulation_history);

        if (seed != -1) {

            sprintf(log_buffer, "writing trajectory %d to database\n", seed);
            dispatcher_log(dispatcher, log_buffer);

            record_simulation_history(
                dispatcher,
                simulation_history, seed);

        }
        else {

            // checking if we have finished
            flag = false;
            for (i = 0; i < dispatcher->number_of_threads; i++) {
                flag = flag || dispatcher->running[i];
            }

            if (! flag)
                break;

        }
    }

    dispatcher_log(dispatcher, "removing duplicate trajectories...\n");
    // we don't check if simulations already exist in the database.
    // That would be mad slow. Instead, we scan for duplicates
    // and remove them at the very end.
    sqlite3_exec(dispatcher->initial_state_database, sql_remove_duplicate_trajectories, 0, 0, 0);
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

    sqlite3_exec(dispatcher->initial_state_database, "BEGIN", 0, 0, 0);

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
                sqlite3_exec(dispatcher->initial_state_database, "COMMIT", 0, 0, 0);
                sqlite3_exec(dispatcher->initial_state_database, "BEGIN", 0, 0, 0);
            }

        }
        chunk = chunk->next_chunk;
    }

    sqlite3_exec(dispatcher->initial_state_database, "COMMIT", 0, 0, 0);

    // free simulation history once we have inserted it into the db
    free_simulation_history(simulation_history);
}



SimulatorPayload *new_simulator_payload(
    ReactionNetwork *reaction_network,
    HistoryQueue *history_queue,
    SolveType type,
    SeedQueue *seed_queue,
    int step_cutoff,
    bool *running
    ) {

    SimulatorPayload *simulator_payload = calloc(1, sizeof(SimulatorPayload));
    simulator_payload->reaction_network = reaction_network;
    simulator_payload->history_queue = history_queue;
    simulator_payload->type = type;
    simulator_payload->seed_queue = seed_queue;
    simulator_payload->step_cutoff = step_cutoff;
    simulator_payload->running = running;
    return simulator_payload;
}

void free_simulator_payload(SimulatorPayload *simulator_payload) {
    // reaction network, seed queue and history queue
    // get freed as part of the dispatcher
    free(simulator_payload);
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

    // tell the dispatcher that we are finished
    *simulator_payload->running = false;


    free_simulator_payload(simulator_payload);
    pthread_exit(NULL);
}
