#include "reaction_network.h"

void initialize_dependents_node(DependentsNode *dependents_node) {
  dependents_node->number_of_dependents = -1;
  dependents_node->dependents = NULL;
  pthread_mutex_init(&dependents_node->mutex, NULL);
}

void free_dependents_node(DependentsNode *dependents_node) {
  // we don't free dnp because they get initialized as a whole chunk
  if (dependents_node->dependents)
    free(dependents_node->dependents);
  pthread_mutex_destroy(&dependents_node->mutex);
}

char sql_get_metadata[] =
    "SELECT * FROM metadata;";

char sql_get_initial_state[] =
    "SELECT * FROM initial_state;";

char sql_get_reactions[] =
    "SELECT reaction_id, number_of_reactants, number_of_products, "
    "reactant_1, reactant_2, product_1, product_2, rate FROM reactions;";


ReactionNetwork *new_reaction_network(sqlite3 *database) {
    ReactionNetwork *rnp = calloc(1, sizeof(ReactionNetwork));


    sqlite3_stmt *get_metadata_stmt;
    sqlite3_stmt *get_initial_state_stmt;
    sqlite3_stmt *get_reactions_stmt;
    int rc;
    int i;
    int species_index, reaction_index;


    rc = sqlite3_prepare_v2(
        database, sql_get_metadata, -1, &get_metadata_stmt, NULL);

    if (rc != SQLITE_OK) {
        printf("new_reaction_network error %s\n", sqlite3_errmsg(database));
        return NULL;
    }

    // filling in metadata
    sqlite3_step(get_metadata_stmt);
    rnp->number_of_species = sqlite3_column_int(get_metadata_stmt, 0);
    rnp->number_of_reactions = sqlite3_column_int(get_metadata_stmt, 1);
    rnp->factor_zero = sqlite3_column_double(get_metadata_stmt, 2);
    rnp->factor_two = sqlite3_column_double(get_metadata_stmt, 3);
    rnp->factor_duplicate = sqlite3_column_double(get_metadata_stmt, 4);



    // allocate number of reactants array
    rnp->number_of_reactants = calloc(rnp->number_of_reactions, sizeof(int));

    // allocate reactant array
    int *reactants_values = calloc(2 * rnp->number_of_reactions, sizeof(int));
    rnp->reactants = calloc(rnp->number_of_reactions, sizeof(int *));
    for (i = 0; i < rnp->number_of_reactions; i++) {
      rnp->reactants[i] = reactants_values + 2 * i;
    }

    // allocate number of products array
    rnp->number_of_products = calloc(rnp->number_of_reactions, sizeof(int));

    // allocate products array
    int *products_values = calloc(2 * rnp->number_of_reactions, sizeof(int));
    rnp->products = calloc(rnp->number_of_reactions, sizeof(int *));
    for (i = 0; i < rnp->number_of_reactions; i++) {
      rnp->products[i] = products_values + 2 * i;
    }

    //allocate rates array
    rnp->rates = calloc(rnp->number_of_reactions, sizeof(double));


    rc = sqlite3_prepare_v2(
        database, sql_get_reactions, -1, &get_reactions_stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("new_reaction_network error %s\n", sqlite3_errmsg(database));
        return NULL;
    }

    // reading reactions from db
    for (i = 0; i < rnp->number_of_reactions; i++) {
        sqlite3_step(get_reactions_stmt);
        reaction_index = sqlite3_column_int(get_reactions_stmt,0);

        rnp->number_of_reactants[reaction_index] =
            sqlite3_column_int(get_reactions_stmt,1);

        rnp->number_of_products[reaction_index] =
            sqlite3_column_int(get_reactions_stmt,2);

        rnp->reactants[reaction_index][0] =
            sqlite3_column_int(get_reactions_stmt,3);

        rnp->reactants[reaction_index][1] =
            sqlite3_column_int(get_reactions_stmt,4);

        rnp->products[reaction_index][0] =
            sqlite3_column_int(get_reactions_stmt,5);

        rnp->products[reaction_index][1] =
            sqlite3_column_int(get_reactions_stmt,6);

        rnp->rates[reaction_index] =
            sqlite3_column_double(get_reactions_stmt,7);
    }


    // allocate initial state
    rnp->initial_state = calloc(rnp->number_of_species, sizeof(int));

    rc = sqlite3_prepare_v2(
        database, sql_get_initial_state, -1, &get_initial_state_stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("new_reaction_network error %s\n", sqlite3_errmsg(database));
        return NULL;
    }

    // fill initial state
    for (i = 0; i < rnp->number_of_species; i++) {
        sqlite3_step(get_initial_state_stmt);
        species_index = sqlite3_column_int(get_initial_state_stmt,0);
        rnp->initial_state[species_index] =
            sqlite3_column_int(get_initial_state_stmt,1);
    }


    initialize_dependency_graph(rnp);
    initialize_propensities(rnp);



    sqlite3_finalize(get_metadata_stmt);
    sqlite3_finalize(get_reactions_stmt);
    sqlite3_finalize(get_initial_state_stmt);
    return rnp;

}


void free_reaction_network(ReactionNetwork *rnp) {
    free(rnp->number_of_reactants);
    free(rnp->reactants[0]);
    free(rnp->reactants);
    free(rnp->number_of_products);
    free(rnp->products[0]);
    free(rnp->products);
    free(rnp->rates);
    free(rnp->initial_state);
    free(rnp->initial_propensities);

    int i; // reaction index
    for (i = 0; i < rnp->number_of_reactions; i++)
        free_dependents_node(rnp->dependency_graph + i);

    free(rnp->dependency_graph);

    free(rnp);

}

DependentsNode *get_dependency_node(ReactionNetwork *rnp, int index) {
    DependentsNode *node = rnp->dependency_graph + index;

    pthread_mutex_lock(&node->mutex);
    if (! node->dependents)
        compute_dependency_node(rnp, index);

    pthread_mutex_unlock(&node->mutex);
    return node;
}

void compute_dependency_node(ReactionNetwork *rnp, int index) {

    DependentsNode *node = rnp->dependency_graph + index;

    int number_of_dependents_count = 0;
    int j; // reaction index
    int l, m, n; // reactant and product indices

    for (j = 0; j < rnp->number_of_reactions; j++) {
        bool flag = false;

        for (l = 0; l < rnp->number_of_reactants[j]; l++) {
            for (m = 0; m < rnp->number_of_reactants[index]; m++) {
                if (rnp->reactants[j][l] == rnp->reactants[index][m])
                    flag = true;
            }

            for (n = 0; n < rnp->number_of_products[index]; n++) {
                if (rnp->reactants[j][l] == rnp->products[index][n])
                    flag = true;
            }
        }

        if (flag)
            number_of_dependents_count++;
    }

    node->number_of_dependents = number_of_dependents_count;
    node->dependents = calloc(
        number_of_dependents_count,
        sizeof(int));

    int dependents_counter = 0;
    int current_reaction = 0;
    while (dependents_counter < number_of_dependents_count) {
        bool flag = false;
        for (l = 0; l < rnp->number_of_reactants[current_reaction]; l++) {
            for (m = 0; m < rnp->number_of_reactants[index]; m++) {
                if (rnp->reactants[current_reaction][l] == rnp->reactants[index][m])
                    flag = true;
            }

            for (n = 0; n < rnp->number_of_products[index]; n++) {
                if (rnp->reactants[current_reaction][l] == rnp->products[index][n])
                    flag = true;
            }
        }

        if (flag) {
            node->dependents[dependents_counter] = current_reaction;
            dependents_counter++;
        }
        current_reaction++;
    }

}

void initialize_dependency_graph(ReactionNetwork *rnp) {


    int i; // reaction index
    rnp->dependency_graph = calloc(
        rnp->number_of_reactions,
        sizeof(DependentsNode)
        );

    for (i = 0; i < rnp->number_of_reactions; i++) {
        initialize_dependents_node(rnp->dependency_graph + i);
    }

    // this is how you would precompute the dependency graph.
    // Doing so is completely inpractical for large networks.
    // for (i = 0; i < rnp->number_of_reactions; i++) {
    //   compute_dependency_node(rnp, i);
    // }

}

double compute_propensity(ReactionNetwork *rnp,
                         int *state,
                         int reaction) {
    double p;
    // zero reactants
    if (rnp->number_of_reactants[reaction] == 0)
        p = rnp->factor_zero
            * rnp->rates[reaction];

    // one reactant
    else if (rnp->number_of_reactants[reaction] == 1)
        p = state[rnp->reactants[reaction][0]]
            * rnp->rates[reaction];


    // two reactants
    else {
        if (rnp->reactants[reaction][0] == rnp->reactants[reaction][1])
            p = rnp->factor_duplicate
                * rnp->factor_two
                * state[rnp->reactants[reaction][0]]
                * (state[rnp->reactants[reaction][0]] - 1)
                * rnp->rates[reaction];

        else
            p = rnp->factor_two
                * state[rnp->reactants[reaction][0]]
                * state[rnp->reactants[reaction][1]]
                * rnp->rates[reaction];

    }

    return p;
}

void initialize_propensities(ReactionNetwork *rnp) {
    rnp->initial_propensities = calloc(rnp->number_of_reactions, sizeof(double));
    for (int reaction = 0; reaction < rnp->number_of_reactions; reaction++) {
        rnp->initial_propensities[reaction] =
            compute_propensity(rnp, rnp->initial_state, reaction);
    }
}
