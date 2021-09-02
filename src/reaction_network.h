#ifndef REACTION_NETWORK_H
#define REACTION_NETWORK_H

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdio.h>


typedef struct dependentsNode {
    int number_of_dependents; // number of reactions that depend on current reaction.
    // number_of_dependents is set to -1 if reaction hasn't been encountered before
    int *dependents; // reactions which depend on current reaction.
    // dependents is set to NULL if dependents need to be computed.
    pthread_mutex_t mutex; // mutex needed because simulation thread initialize dependents
    int number_of_occurrences; // number of times the reaction has occoured.
} DependentsNode;

// struct for storing the static reaction network state which
// will be shared across all simulation instances

void initialize_dependents_node(DependentsNode *dependents_node);
void free_dependents_node(DependentsNode *dependents_node);

typedef struct reactionNetwork {

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

ReactionNetwork *new_reaction_network(sqlite3 *database);
void free_reaction_network(ReactionNetwork *reaction_network);

DependentsNode *get_dependency_node(ReactionNetwork *reaction_network, int index);

// expensive operation where we loop through the dependency graph and free
// dependency nodes which have been used only once
int garbage_collect_dependency_graph(
    ReactionNetwork *reaction_network,
    int gc_threshold);


void compute_dependency_node(ReactionNetwork *reaction_network, int reaction);
void initialize_dependency_graph(ReactionNetwork *reaction_network);


double compute_propensity(ReactionNetwork *rnp, int *state, int reaction);
void initialize_propensities(ReactionNetwork *rnp);


#endif
