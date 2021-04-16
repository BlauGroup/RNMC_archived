#ifndef REACTION_NETWORK_H
#define REACTION_NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <sqlite3.h>

typedef struct dependentsNode {
  int number_of_dependents; // number of reactions that depend on current reaction.
  // number_of_dependents is set to -1 if reaction hasn't been encountered before
  int *dependents; // reactions which depend on current reaction.
  // dependents is set to NULL if reaction hasn't been encountered before
  pthread_mutex_t mutex; // mutex needed because simulation thread initialize dependents
  time_t first_observed; // time in seconds since start of program when reaction first observed
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
  time_t start_time; // start time of simulation
  bool logging;
} ReactionNetwork;

ReactionNetwork *new_reaction_network_from_files(char *directory, bool logging);
ReactionNetwork *new_reaction_network_from_db(char *directory, bool logging);
ReactionNetwork *new_reaction_network(char *directory, bool logging);
void free_reaction_network(ReactionNetwork *rnp);

DependentsNode *get_dependency_node(ReactionNetwork *rnp, int index);
void compute_dependency_node(ReactionNetwork *rnp, int reaction);
void initialize_dependency_graph(ReactionNetwork *rnp);


double compute_propensity(ReactionNetwork *rnp, int *state, int reaction);
void initialize_propensities(ReactionNetwork *rnp);

// serialize a reaction network to the disk (not including initial propensities
// and dependency graph). Each attribute is stored in directory/attribute_name
// directory is an argument so we can serialize into another location
int reaction_network_to_files(ReactionNetwork *rnp, char *directory);

// like reaction_network_to_files except we serialize the reaction network to a
// sqlite database.
int reaction_network_to_db(ReactionNetwork *rnp, char *directory, int shard_size);


bool reaction_networks_differ(ReactionNetwork *rnpa, ReactionNetwork *rnpb);

#endif
