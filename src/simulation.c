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

void free_simulation_history(SimulationHistory *simulation_historyp) {
  Chunk *chunkp = simulation_historyp->first_chunk;
  Chunk *next_chunkp;

  while (chunkp) {
    next_chunkp = chunkp->next_chunk;
    free(chunkp);
    chunkp = next_chunkp;
  }

  free(simulation_historyp);
}

void insert_history_element(SimulationHistory *simulation_historyp, int reaction, double time) {

  Chunk *last_chunkp = simulation_historyp->last_chunk;
  if (last_chunkp->next_free_index == CHUNK_SIZE) {
    Chunk *new_chunkp = new_chunk();
    last_chunkp->next_chunk = new_chunkp;
    simulation_historyp->last_chunk = new_chunkp;
    new_chunkp->data[0].reaction = reaction;
    new_chunkp->data[0].time = time;
    new_chunkp->next_free_index++;
  } else {
    last_chunkp->data[last_chunkp->next_free_index].reaction = reaction;
    last_chunkp->data[last_chunkp->next_free_index].time = time;
    last_chunkp->next_free_index++;
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


Simulation *new_simulation(ReactionNetwork *rnp,
                           unsigned long int seed,
                           SolveType type) {
  int i;


  Simulation *sp = calloc(1, sizeof(Simulation));
  sp->rn = rnp;
  sp->seed = seed;
  sp->state = calloc(rnp->number_of_species, sizeof(int));

  for (i = 0; i < rnp->number_of_species; i++)
    sp->state[i] = rnp->initial_state[i];

  sp->time = 0.0;
  sp->step = 0;
  sp->solver = new_solve(type,
                         seed,
                         rnp->number_of_reactions,
                         rnp->initial_propensities);

  sp->history = new_simulation_history();

  return sp;
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
    for (m = 0; m < simulation->rn->number_of_reactants[next_reaction]; m++)
      simulation->state[simulation->rn->reactants[next_reaction][m]]--;
    for (m = 0; m < simulation->rn->number_of_products[next_reaction]; m++)
      simulation->state[simulation->rn->products[next_reaction][m]]++;

    // update propensities
    DependentsNode *dnp = get_dependency_node(simulation->rn, next_reaction);
    int number_of_updates = dnp->number_of_dependents;
    int *dependents = dnp->dependents;

    for (m = 0; m < number_of_updates; m++) {
      reaction_index = dependents[m];
      new_propensity = compute_propensity(
          simulation->rn,
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

bool check_state_positivity(Simulation *sp) {
  int i;
  for (i = 0; i < sp->rn->number_of_species; i++) {
    if (sp->state[i] < 0) {
      puts("negative state encountered!!!");
      return false;
    }
  }
  return true;
}

