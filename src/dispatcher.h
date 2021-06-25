#ifndef DISPATCHER_H
#define DISPATCHER_H


#include <pthread.h>
#include <sqlite3.h>
#include "reaction_network.h"
#include "simulation.h"


typedef struct seedQueue {
    unsigned int *seeds;
    int number_of_seeds; // length of seeds array
    int next_seed; // index into seeds array
    pthread_mutex_t mtx;
} SeedQueue;

SeedQueue *new_seed_queue(int number_of_seeds, unsigned int base_seed);
void free_seed_queue(SeedQueue *sqp);
unsigned long int get_seed(SeedQueue *sqp);


typedef struct historyNode {
    SimulationHistory *simulation_history;
    struct historyNode *next;
} HistoryNode;

HistoryNode *new_history_node(SimulationHistory *simulation_history);

// simulation history shouldn't be freed here.
// will be freed by dispatcher
void free_history_node(HistoryNode *hnp);

// emptry if history_node == NULL
typedef struct historyQueue {
    HistoryNode *history_node;
    pthread_mutex_t mtx;
} HistoryQueue;

HistoryQueue *new_history_queue();

// history queue should never be freed if not empty, since then
// we wouldn't log those histories in the database
void free_history_queue(HistoryQueue *hqp);


void insert_simulation_history(HistoryQueue *hqp, SimulationHistory *shp);
SimulationHistory *get_simulation_history(HistoryQueue *hqp);

typedef struct dispatcher {
    sqlite3 *db;
    ReactionNetwork *rn;
    HistoryQueue *hq;
    SeedQueue *sq;
    int number_of_threads; // length of threads array
    pthread_t *threads;
    int step_cutoff; // step cutoff
    bool logging; // logging enabled

} Dispatcher;

Dispatcher *new_dispatcher(
    char *database_file,
    int number_of_simulations,
    int base_seed,
    int number_of_threads,
    int step_cutoff,
    bool logging);

void free_dispatcher(Dispatcher *dp);
void run_dispatcher(Dispatcher *dp);


typedef struct simulatorPayload {
    ReactionNetwork *rn;
    HistoryQueue *hq;
    SolveType type;
    SeedQueue *sq;
    int step_cutoff;
} SimulatorPayload;

SimulatorPayload *new_simulator_payload(
    ReactionNetwork *rn,
    HistoryQueue *hq,
    SolveType type,
    SeedQueue *sq,
    int step_cutoff
    );

void free_simulator_payload(SimulatorPayload *sp);
void *run_simulator(void *simulator_payload);




#endif
