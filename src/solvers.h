#ifndef SOLVERS_H
#define SOLVERS_H
#include "sampler.h"
#include <math.h>
#include <stdlib.h>

/***************************************************************************/
/* generic solver                                                          */
/* the solver is the algorithmic backbone of a reaction network simulation */
/* it decides which reaction will occour next.                             */
/*                                                                         */
/* for now, we have the linear solver and a tree solver ported from        */
/* spparks: https://spparks.sandia.gov/                                    */
/***************************************************************************/

typedef enum solveType {
  linear,
  tree,
} SolveType;

// General solver API. this allows us to swap out the solver and add new solvers
typedef struct solveGeneral {

  void (*update)(void *p,
                 int reaction_to_update,
                 double new_propensity);

  // reactions_to_update is a pointer to the indices to update
  void (*update_many)(void *p,
                     int number_of_updates,
                     int *reactions_to_update,
                     double *propensity_buffer);

  int (*event)(void *p, double *dtp);

  double (*get_propensity)(void *p, int reaction);

  double (*get_propensity_sum)(void *p);

  int (*get_number_of_active_reactions)(void *p);

  SolveType type;

} Solve;

Solve *new_solve(SolveType type,
                unsigned long int seed,
                int number_of_reactions,
                double *initial_propensities);

void free_solve(Solve *p);


// linear solver

typedef struct solveLinear {

  // API
  void (*update)(void *solve_linearp,
                 int reaction_to_update,
                 double new_propensity);

  void (*update_many)(void *solve_linearp,
                     int number_of_updates,
                     int *reactions_to_update,
                     double *propensity_buffer);

  int (*event)(void *solve_linearp, double *dtp);

  double (*get_propensity)(void *solve_linearp, int reaction);

  double (*get_propensity_sum)(void *solve_linearp);

  int (*get_number_of_active_reactions)(void *solve_linearp);

  SolveType type;

  // internal state
  Sampler *sampler;
  int number_of_reactions;
  int number_of_active_reactions;
  double *propensities;  // propensities stored as a linear array
  double propensity_sum;

} SolveLinear;

SolveLinear *new_solve_linear(unsigned long int seed,
                              int number_of_reactions,
                              double *initial_propensities);

void free_solve_linear(SolveLinear *p);

void update_solve_linear(void *solve_linearp,
                         int reaction_to_update,
                         double new_propensity);

void update_many_solve_linear(void *solve_linearp,
                              int number_of_updates,
                              int *reactions_to_update,
                              double *propensity_buffer);

int event_solve_linear(void *solve_linearp, double *dtp);

double get_propensity_solve_linear(void *solve_linearp, int reaction);
double get_propensity_sum_solve_linear(void *solve_linearp);
int get_number_of_active_reactions_solve_linear(void *solve_linearp);

// tree solver

typedef struct solveTree {

  // API
  void (*update)(void *solve_treep,
                 int reaction_to_update,
                 double new_propensity);

  void (*update_many)(void *solve_treep,
                     int number_of_updates,
                     int *reactions_to_update,
                     double *propensity_buffer);

  int (*event)(void *solve_treep, double *dtp);

  double (*get_propensity)(void *solve_treep, int reaction);

  double (*get_propensity_sum)(void *solve_treep);

  int (*get_number_of_active_reactions)(void *solve_treep);

  SolveType type;

  // internal state
  Sampler *sampler;
  int number_of_reactions;
  int number_of_active_reactions;
  int number_of_tree_nodes;
  int propensity_offset; // index where propensities start as leaves of tree
  double *tree;  // propensities stored as a binary heap
  double propensity_sum;
} SolveTree;


SolveTree *new_solve_tree(unsigned long int seed,
                        int number_of_reactions,
                        double *initial_propensities);

void free_solve_tree(SolveTree *p);

void sum_solve_tree(SolveTree *p);
int find_solve_tree(SolveTree *p, double value);

void update_solve_tree(void *solve_treep,
                     int reaction_to_update,
                     double new_propensity);

void update_many_solve_tree(void *solve_treep,
                         int number_of_updates,
                         int *reactions_to_update,
                         double *propensity_buffer);

int event_solve_tree(void *solve_treep, double *dtp);

double get_propensity_solve_tree(void *solve_treep, int reaction);
double get_propensity_sum_solve_tree(void *solve_treep);
int get_number_of_active_reactions_solve_tree(void *solve_treep);

#endif
