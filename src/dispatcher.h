#ifndef DISPATCHER_H
#define DISPATCHER_H


#include <pthread.h>
#include <sqlite3.h>
#include "reaction_network.h"



typedef struct seedQueue {
    unsigned int *seeds;
    int number_of_seeds; // length of seeds array
    int next_seed; // index into seeds array
    pthread_mutex_t mtx;
} SeedQueue;

SeedQueue *new_seed_queue(int number_of_seeds, unsigned int base_seed);
void free_seed_queue(SeedQueue *sqp);
unsigned long int get_seed(SeedQueue *sqp);


typedef struct dispatcher {
    sqlite3 *db;
    ReactionNetwork *rn;
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




#endif
