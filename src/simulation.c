#include "simulation.h"

Chunk *new_chunk() {
    Chunk *chunkp = calloc(1, sizeof(Chunk));
    int i;
    for (i = 0; i < CHUNK_SIZE; i++) {
        chunkp->data[i].reaction = -1;
        chunkp->data[i].time = 0.0;
    }
    chunkp->next_free_index = 0;
    chunkp->next_chunk = NULL;
    return chunkp;
}

SimulationHistory *new_simulation_history() {
    SimulationHistory *simulation_historyp = calloc(1, sizeof(SimulationHistory));
    Chunk *chunkp = new_chunk();
    simulation_historyp->first_chunk = chunkp;
    simulation_historyp->last_chunk = chunkp;

    return simulation_historyp;
}

void free_simulation_history(SimulationHistory *simulation_history) {
  Chunk *chunk = simulation_history->first_chunk;
  Chunk *next_chunk;

  while (chunk) {
    next_chunk = chunk->next_chunk;
    free(chunk);
    chunk = next_chunk;
  }

  free(simulation_history);
}

void insert_history_element(
    SimulationHistory *simulation_history,
    int reaction,
    double time) {

    Chunk *last_chunk = simulation_history->last_chunk;
    if (last_chunk->next_free_index == CHUNK_SIZE) {
        Chunk *new_chunkp = new_chunk();
        last_chunk->next_chunk = new_chunkp;
        simulation_history->last_chunk = new_chunkp;
        new_chunkp->data[0].reaction = reaction;
        new_chunkp->data[0].time = time;
        new_chunkp->next_free_index++;
    } else {
        last_chunk->data[last_chunk->next_free_index].reaction = reaction;
        last_chunk->data[last_chunk->next_free_index].time = time;
        last_chunk->next_free_index++;
  }
}

int simulation_history_length(SimulationHistory *shp) {
  int length = 0;
  Chunk *chunk = shp->first_chunk;
  while (chunk) {
    length += chunk->next_free_index;
    chunk = chunk->next_chunk;
  }
  return length;
}


Simulation *new_simulation(ReactionNetwork *reaction_network,
                           unsigned long int seed,
                           SolveType type) {
  int i;


  Simulation *simulation = calloc(1, sizeof(Simulation));
  simulation->reaction_network = reaction_network;
  simulation->seed = seed;
  simulation->state = calloc(reaction_network->number_of_species, sizeof(int));

  for (i = 0; i < reaction_network->number_of_species; i++)
    simulation->state[i] = reaction_network->initial_state[i];

  simulation->time = 0.0;
  simulation->step = 0;
  simulation->solver = new_solve(type,
                         seed,
                         reaction_network->number_of_reactions,
                         reaction_network->initial_propensities);

  simulation->history = new_simulation_history();

  return simulation;
}

void free_simulation(Simulation *simulation) {
  // don't free the reaction network as it is shared by all simulations
  // don't free the simulation history as it gets transfered to and
  // freed by the dispatcher
  free(simulation->state);
  free_solve(simulation->solver);
  free(simulation);
}

bool step(Simulation *simulation) {
  int m;
  double dt;
  bool dead_end = false;
  int next_reaction = simulation->solver->event(simulation->solver, &dt);
  int reaction_index;
  double new_propensity;

  if (next_reaction < 0) dead_end = true;
  else {
    // update steps and time
    simulation->step++;
    simulation->time += dt;

    // record reaction
    insert_history_element(
        simulation->history,
        next_reaction,
        simulation->time);

    // update state
    for (m = 0;
         m < simulation->reaction_network->number_of_reactants[next_reaction];
         m++)

      simulation->state[simulation->reaction_network->reactants[next_reaction][m]]--;

    for (m = 0;
         m < simulation->reaction_network->number_of_products[next_reaction];
         m++)

      simulation->state[simulation->reaction_network->products[next_reaction][m]]++;

    // update propensities
    DependentsNode *dnp = get_dependency_node(
        simulation->reaction_network,
        next_reaction);

    int number_of_updates = dnp->number_of_dependents;
    int *dependents = dnp->dependents;

    for (m = 0; m < number_of_updates; m++) {
      reaction_index = dependents[m];
      new_propensity = compute_propensity(
          simulation->reaction_network,
          simulation->state,
          reaction_index);

      simulation->solver->update(
          simulation->solver,
          reaction_index,
          new_propensity);

    }


  }

  return dead_end;
}


void run_for(Simulation *simulation, int step_cutoff) {
  while (!step(simulation)) {
    if (simulation->step > step_cutoff)
      break;
  }
}

bool check_state_positivity(Simulation *simulation) {
  int i;
  for (i = 0; i < simulation->reaction_network->number_of_species; i++) {
    if (simulation->state[i] < 0) {
      puts("negative state encountered!!!");
      return false;
    }
  }
  return true;
}

