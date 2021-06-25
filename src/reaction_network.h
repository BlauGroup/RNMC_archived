#ifndef REACTION_NETWORK_H
#define REACTION_NETWORK_H

#include <pthread.h>
#include <time.h>
#include <stdbool.h>

typedef struct dependentsNode {
  int number_of_dependents; // number of reactions that depend on current reaction.
  // number_of_dependents is set to -1 if reaction hasn't been encountered before
  int *dependents; // reactions which depend on current reaction.
  // dependents is set to NULL if reaction hasn't been encountered before
  pthread_mutex_t mutex; // mutex needed because simulation thread initialize dependents
} DependentsNode;

// struct for storing the static reaction network state which
// will be shared across all simulation instances

DependentsNode *new_dependents_node();
void free_dependents_node(DependentsNode *dnp);

typedef struct reactionNetwork {

  // filled out by reaction_network_from_file
  char *dir; // directory where reaction network serialization files are
  int number_of_species;
  int number_of_reactions;
  // we assume that each reaction has zero, one or two reactants
  int *number_of_reactants; // array storing the number of reactants of each reaction
  int **reactants; // array storing the reactants of each reaction
  int *number_of_products; // array storing the number of products of each reaction
  int **products; // array storing the products of each reaction


  double factor_zero; // rate modifer for reactions with zero reactants
  double factor_two; // rate modifier for reactions with two reactants
  double factor_duplicate; // rate modifier for reactions of form A + A -> ...
  double *rates; // array storing the rates for each reaction


  int *initial_state; // initial state for all the simulations


  // allocated and filled out by initializePropensities. Initially set to 0
  double *initial_propensities; // initial propensities for all the reactions


  // dependency graph. List of DependencyNodes number_of_reactions long.
  DependentsNode *dependency_graph;
} ReactionNetwork;



#endif
