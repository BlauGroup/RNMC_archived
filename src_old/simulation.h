#ifndef SIMULATION_H
#define SIMULATION_H

#include "reaction_network.h"
#include "solvers.h"
#include <pthread.h>

#define CHUNK_SIZE 1024

typedef struct historyElement {
    int reaction;
    double time;
} HistoryElement;

typedef struct chunk {
  HistoryElement data[CHUNK_SIZE];
  int next_free_index;
  struct chunk *next_chunk;
} Chunk;

// Chunks are never freed by themselves.
// always freed as part of a simulation history
Chunk *new_chunk();

typedef struct simulationHistory {
  Chunk *first_chunk;
  Chunk *last_chunk;
} SimulationHistory;

SimulationHistory *new_simulation_history();
void free_simulation_history(SimulationHistory *simulation_historyp);
void insert_history_element(SimulationHistory *simulation_historyp, int reaction, double time);
int simulation_history_length(SimulationHistory *shp);

typedef struct simulation {
  ReactionNetwork *rn;
  unsigned long int seed;
  int *state;
  double time;
  int step; // number of reactions which have occurred
  Solve *solver;
  SimulationHistory *history;
} Simulation;

Simulation *new_simulation(ReactionNetwork *rnp,
                           unsigned long int seed,
                           SolveType type);

void free_simulation(Simulation *sp);

bool step(Simulation *sp);
void run_until(Simulation *sp, double time_cutoff);
void run_for(Simulation *sp, int step_cutoff);
void simulation_history_to_file(Simulation *sp);

bool check_state_positivity(Simulation *sp);

#endif
