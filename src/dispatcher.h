#ifndef DISPATCHER_H
#define DISPATCHER_H


#include <pthread.h>
#include <sqlite3.h>
#include <time.h>
#include "reaction_network.h"
#include "simulation.h"


typedef struct seedQueue {
    unsigned int *seeds;
    int number_of_seeds; // length of seeds array
    int next_seed; // index into seeds array
    pthread_mutex_t mutex;
} SeedQueue;

SeedQueue *new_seed_queue(int number_of_seeds, unsigned int base_seed);
void free_seed_queue(SeedQueue *seed_queue);
unsigned long int get_seed(SeedQueue *seed_queue);


typedef struct historyNode {
    SimulationHistory *simulation_history;
    int seed;
    struct historyNode *next;
} HistoryNode;

HistoryNode *new_history_node(SimulationHistory *simulation_history, int seed);

// simulation history shouldn't be freed here.
// will be freed by dispatcher
void free_history_node(HistoryNode *history_node);

// emptry if history_node == NULL
typedef struct historyQueue {
    HistoryNode *history_node;
    pthread_mutex_t mutex;
} HistoryQueue;

HistoryQueue *new_history_queue();

// history queue should never be freed if not empty, since then
// we wouldn't log those histories in the database
void free_history_queue(HistoryQueue *hqp);


void insert_simulation_history(HistoryQueue *hqp, SimulationHistory *shp, int seed);

// get a simulation history and return the seed
// if none to be found, return -1
int get_simulation_history(
    HistoryQueue *history_queue,
    SimulationHistory **simulation_history);

typedef struct dispatcher {
    sqlite3 *reaction_database;
    sqlite3 *initial_state_database;
    sqlite3_stmt *insert_trajectory_stmt;
    ReactionNetwork *reaction_network;
    HistoryQueue *history_queue;
    SeedQueue *seed_queue;
    int number_of_threads; // length of threads array
    pthread_t *threads;
    bool *running;   // array of bools indicating which threads are still running
    int step_cutoff; // step cutoff
    bool logging; // logging enabled
    long int start_time;
    long int gc_interval;
    int gc_threshold;

} Dispatcher;

Dispatcher *new_dispatcher(
    char *reaction_database_file,
    char *initial_state_database_file,
    int number_of_simulations,
    int base_seed,
    int number_of_threads,
    int step_cutoff,
    long int gc_interval,
    int gc_threshold,
    bool logging);

void free_dispatcher(Dispatcher *dispatcher);
void run_dispatcher(Dispatcher *dispatcher);

// if you want to pass a format string to dispatcher_log, first
// evaluate it into a buffer using sprintf and then pass that buffer
// to dispatcher_log
void dispatcher_log(Dispatcher *dispatcher, char *message);


#define TRANSACTION_SIZE 10000

void record_simulation_history(
    Dispatcher *dispatcher,
    SimulationHistory *simulation_history,
    int seed);


typedef struct simulatorPayload {
    ReactionNetwork *reaction_network;
    HistoryQueue *history_queue;
    SolveType type;
    SeedQueue *seed_queue;
    int step_cutoff;
    // pointer to a bool shared with dispatcher
    // so it can query if we are still running
    bool *running;
} SimulatorPayload;

SimulatorPayload *new_simulator_payload(
    ReactionNetwork *reaction_network,
    HistoryQueue *history_queue,
    SolveType type,
    SeedQueue *seed_queue,
    int step_cutoff,
    bool *running
    );

void free_simulator_payload(SimulatorPayload *simulator_payload);
void *run_simulator(void *simulator_payload);




#endif
