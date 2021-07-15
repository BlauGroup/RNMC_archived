#ifndef SIMULATION_H
#define SIMULATION_H


#include <pthread.h>
#include "reaction_network.h"
#include "solvers.h"

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
void free_simulation_history(SimulationHistory *simulation_history);
void insert_history_element(SimulationHistory *simulation_history, int reaction, double time);
int simulation_history_length(SimulationHistory *simulation_history);

typedef struct simulation {
  ReactionNetwork *reaction_network;
  unsigned long int seed;
  int *state;
  double time;
  int step; // number of reactions which have occurred
  Solve *solver;
  SimulationHistory *history;
} Simulation;

Simulation *new_simulation(ReactionNetwork *reaction_network,
                           unsigned long int seed,
                           SolveType type);

// the simulation history is passed to the dispatcher
// don't free it when freeing the simulation state
void free_simulation(Simulation *simulation);

bool step(Simulation *simulation);
void run_for(Simulation *simulation, int step_cutoff);
bool check_state_positivity(Simulation *simulation);

#endif
