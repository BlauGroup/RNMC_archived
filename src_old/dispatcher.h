#include "reaction_network.h"
#include "simulation.h"
#include <pthread.h>
#include <errno.h>

// seed 0 is special. It is the value returned when the seed queue is empty
// this means that you should never have 0 as a seed value because as soon
// as it gets passed to a simulator thread, it will terminate.
typedef struct seedQueue {
  unsigned long int *seeds;
  int number_of_seeds; // length of seeds array
  int next_seed; // index into seeds array
  pthread_mutex_t mtx;
} SeedQueue;

SeedQueue *new_seed_queue(int number_of_seeds, unsigned long int *seeds);
void free_seed_queue(SeedQueue *sqp);
unsigned long int get_seed(SeedQueue *sqp);

typedef enum cutoffType { time_cutoff, step_cutoff } CutoffType;

typedef struct dispatcher {
  char *reaction_network_dir;
  ReactionNetwork *rn;

  char *simulation_params;
  // simulation_params dir directory contains three files
  // number_of_threads which contains the number of simulation threads
  // number_of_seeds which contains the number of seeds
  // seeds which is a list of new line delimited seeds
  // time_cutoff is how long to run each simulation for (in simulation time)
  SeedQueue *sq;
  int number_of_threads; // length of threads array
  pthread_t *threads;
  bool logging; // logging enabled
  CutoffType cutoff_type;
  double time_cutoff;
  int step_cutoff;

} Dispatcher;

Dispatcher *new_dispatcher(char *reaction_network_dir, char *simulation_params, bool logging);
void free_dispatcher(Dispatcher *dp);
void run_dispatcher(Dispatcher *dp);


typedef struct simulatorPayload {
  ReactionNetwork *rn;
  SolveType type;
  SeedQueue *sq;
  CutoffType cutoff_type;
  double time_cutoff;
  int step_cutoff;
} SimulatorPayload;

SimulatorPayload *new_simulator_payload(ReactionNetwork *rn,
                                        SolveType type,
                                        SeedQueue *sq,
                                        CutoffType cutoff_type,
                                        double time_cutoff,
                                        int step_cutoff
                                        );

void free_simulator_payload(SimulatorPayload *sp);
void *run_simulator(void *simulator_payload);
