#include "solvers.h"
#include <signal.h>

// generic solve

Solve *new_solve(SolveType type,
                unsigned long int seed,
                int number_of_reactions,
                double *initial_propensities) {
  switch (type) {
  case linear:
    return (Solve *) new_solve_linear(seed, number_of_reactions, initial_propensities);

  case tree:
    return (Solve *) new_solve_tree(seed, number_of_reactions, initial_propensities);
  }

  return NULL;
}

void free_solve(Solve *p) {
  switch (p->type){
  case linear:
    free_solve_linear((SolveLinear *) p);
    break;

  case tree:
    free_solve_tree((SolveTree *) p);
    break;
    }
}

// linear solver

SolveLinear *new_solve_linear(unsigned long int seed,
                            int number_of_reactions,
                            double *initial_propensities) {

    SolveLinear *p = calloc(1, sizeof(SolveLinear));
    p->update = &update_solve_linear;
    p->update_many = &update_many_solve_linear;
    p->event = &event_solve_linear;
    p->get_propensity = &get_propensity_solve_linear;
    p->get_propensity_sum = &get_propensity_sum_solve_linear;
    p->get_number_of_active_reactions = &get_number_of_active_reactions_solve_linear;
    p->type = linear;
    p->sampler = new_sampler(seed);
    p->number_of_reactions = number_of_reactions;
    p->number_of_active_reactions = 0;
    p->propensities = calloc(number_of_reactions, sizeof(double));
    p->propensity_sum = 0.0;

    for (int i = 0; i < number_of_reactions; i++) {
        if (initial_propensities[i] > 0.0) p->number_of_active_reactions++;
        p->propensities[i] = initial_propensities[i];
        p->propensity_sum += initial_propensities[i];
    }

    return p;
}

void free_solve_linear(SolveLinear *p){
  free_sampler(p->sampler);
  free(p->propensities);
  free(p);
}

void update_solve_linear(void *solve_linearp,
                         int reaction_to_update,
                         double new_propensity) {
  SolveLinear *p = (SolveLinear *) solve_linearp;
  if (p->propensities[reaction_to_update] > 0.0) p->number_of_active_reactions--;
  if (new_propensity > 0.0) p->number_of_active_reactions++;
  p->propensity_sum -= p->propensities[reaction_to_update];
  p->propensity_sum += new_propensity;
  p->propensities[reaction_to_update] = new_propensity;
}

void update_many_solve_linear(void *solve_linearp,
                              int number_of_updates,
                              int *reactions_to_update,
                              double *new_propensities) {
  SolveLinear *p = (SolveLinear *) solve_linearp;
  for (int i = 0; i < number_of_updates; i++) {
    int reaction_to_update = reactions_to_update[i];
    update_solve_linear(p, reaction_to_update, new_propensities[reaction_to_update]);
  }
}

int event_solve_linear(void *solve_linearp, double *dtp) {
  SolveLinear *p = (SolveLinear *) solve_linearp;
  int m;

  if (p->number_of_active_reactions == 0) {
    p->propensity_sum = 0.0;
    return -1;
  }

  double r1 = p->sampler->generate(p->sampler);
  double r2 = p->sampler->generate(p->sampler);

  double fraction = p->propensity_sum * r1;
  double partial = 0.0;

  for (m = 0; m < p->number_of_reactions; m++) {
    partial += p->propensities[m];
    if (partial > fraction) break;
  }

  *dtp = - log(r2) / p->propensity_sum;

  if (m < p->number_of_reactions) return m;
  return p->number_of_reactions - 1;
}

double get_propensity_solve_linear(void *solve_linearp, int reaction) {
   SolveLinear *p = (SolveLinear *) solve_linearp;
   return p->propensities[reaction];
}

double get_propensity_sum_solve_linear(void *solve_linearp) {
  SolveLinear *p = (SolveLinear *) solve_linearp;
  return p->propensity_sum;
}

int get_number_of_active_reactions_solve_linear(void *solve_linearp) {
   SolveLinear *p = (SolveLinear *) solve_linearp;
   return p->number_of_active_reactions;

}

// tree solver

SolveTree *new_solve_tree(unsigned long int seed,
                          int number_of_reactions,
                          double *initial_propensities) {

    SolveTree *p = calloc(1, sizeof(SolveTree));
    p->update = &update_solve_tree;
    p->update_many = &update_many_solve_tree;
    p->event = &event_solve_tree;
    p->get_propensity = &get_propensity_solve_tree;
    p->get_propensity_sum = &get_propensity_sum_solve_tree;
    p->get_number_of_active_reactions = &get_number_of_active_reactions_solve_tree;
    p->type = tree;
    p->sampler = new_sampler(seed);
    p->number_of_reactions = number_of_reactions;
    p->number_of_active_reactions = 0;

    int m = 0; // tree depth
    int pow2 = 1;  // power of 2 >= numberOfReactions

    while (pow2 < number_of_reactions) {
        pow2 *= 2;
        m++;
    }

    p->number_of_tree_nodes = 2 * pow2 - 1;
    p->propensity_offset = pow2 - 1;
    p->tree = calloc(p->number_of_tree_nodes, sizeof(double));

    // initialize tree
    for (int i = 0; i < p->number_of_tree_nodes; i++) p->tree[i] = 0.0;
    for (int i = p->propensity_offset;
         i < p->propensity_offset + number_of_reactions;
         i++) {
        p->tree[i] = initial_propensities[i - p->propensity_offset];
    }
    p->propensity_sum = 0.0;

    // finish initializing the tree
    // set propensitySum
    // compute number of active reactions
    sum_solve_tree(p);


  return p;
}

void free_solve_tree(SolveTree *p) {
  free_sampler(p->sampler);
  free(p->tree);
  free(p);
}

void sum_solve_tree(SolveTree *p) {
  // initialize the tree
  // this function loops through all reactions
  // should never be run during an active simulation
  int child1, child2;
  for (int parent = p->propensity_offset - 1; parent >= 0; parent--) {
    child1 = 2 * parent + 1;
    child2 = 2 * parent + 2;
    p->tree[parent] = p->tree[child1] + p->tree[child2];
  }

  p->propensity_sum = p->tree[0];
  p->number_of_active_reactions = 0;

  for (int i = p->propensity_offset; i < p->number_of_tree_nodes; i++)
    if (p->tree[i] > 0.0) p->number_of_active_reactions++;


}

void update_solve_tree(void *solve_treep, int reaction_to_update,
                     double new_propensity) {

  // update number of active reactions
  SolveTree *p = (SolveTree *) solve_treep;
  if (p->tree[p->propensity_offset + reaction_to_update] > 0.0)
    p->number_of_active_reactions--;
  if (new_propensity > 0.0)
    p->number_of_active_reactions++;

  // set new propensity
  p->tree[p->propensity_offset + reaction_to_update] = new_propensity;

  // propogate new propensity up to root
  int parent, sibling;
  int i = p->propensity_offset + reaction_to_update;

  while (i > 0) {
    if (i % 2) sibling = i + 1;
    else sibling = i - 1;
    parent = (i - 1) / 2;
    p->tree[parent] = p->tree[i] + p->tree[sibling];
    i = parent;
  }

  // update total propensity
  p->propensity_sum = p->tree[0];
}

void update_many_solve_tree(void *solve_treep,
                         int number_of_updates,
                         int *reactions_to_update,
                         double *new_propensities) {
  for (int i = 0; i < number_of_updates; i++) {
    SolveTree *p = (SolveTree *) solve_treep;
    int reaction_to_update = reactions_to_update[i];
    update_solve_tree(p, reaction_to_update, new_propensities[reaction_to_update]);
  }
}

int find_solve_tree(SolveTree *p, double value) {
  int i, left_child;

  // walk tree from root to appropriate leaf
  // value is modified when right branch of tree is traversed

  i = 0;
  while (i < p->propensity_offset) {
    left_child = 2*i + 1;
    if (value <= p->tree[left_child]) i = left_child;
    else {
      value -= p->tree[left_child];
      i = left_child + 1;
    }
  }
  return i - p->propensity_offset;

}

int event_solve_tree(void *solve_treep, double *dtp) {

  SolveTree *p = (SolveTree *) solve_treep;

  int m;
  double r1,r2;


  if (p->number_of_active_reactions == 0) {
    return -1;
  }


  r1 = p->sampler->generate(p->sampler);
  r2 = p->sampler->generate(p->sampler);

  double value = r1 * p->propensity_sum;

  m = find_solve_tree(p,value);
  *dtp = - log(r2) / p->propensity_sum;

  return m;

}

double get_propensity_solve_tree(void *solve_treep, int reaction) {
  SolveTree *p = (SolveTree *) solve_treep;
  return p->tree[p->propensity_offset + reaction];
}



double get_propensity_sum_solve_tree(void *solve_treep) {
  SolveTree *p = (SolveTree *) solve_treep;
  return p->propensity_sum;
}


int get_number_of_active_reactions_solve_tree(void *solve_treep) {
  SolveTree *p = (SolveTree *) solve_treep;
  return p->number_of_active_reactions;

}
