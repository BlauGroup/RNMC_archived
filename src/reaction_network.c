#include "reaction_network.h"

void initialize_dependents_node(DependentsNode *dependents_node) {
    dependents_node->number_of_dependents = -1;
    dependents_node->dependents = NULL;
    dependents_node->number_of_occurrences = 0;
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


ReactionNetwork *new_reaction_network(
    sqlite3 *reaction_network_database,
    sqlite3 *initial_state_database,
    int dependency_threshold
    ) {
    ReactionNetwork *reaction_network = calloc(1, sizeof(ReactionNetwork));


    sqlite3_stmt *get_metadata_stmt;
    sqlite3_stmt *get_initial_state_stmt;
    sqlite3_stmt *get_reactions_stmt;
    int rc;
    int i;
    int species_index, reaction_index;


    rc = sqlite3_prepare_v2(
        reaction_network_database, sql_get_metadata, -1, &get_metadata_stmt, NULL);

    if (rc != SQLITE_OK) {
        printf("new_reaction_network error %s\n",
               sqlite3_errmsg(reaction_network_database));
        return NULL;
    }

    // filling in metadata
    sqlite3_step(get_metadata_stmt);
    reaction_network->number_of_species = sqlite3_column_int(get_metadata_stmt, 0);
    reaction_network->number_of_reactions = sqlite3_column_int(get_metadata_stmt, 1);
    reaction_network->factor_zero = sqlite3_column_double(get_metadata_stmt, 2);
    reaction_network->factor_two = sqlite3_column_double(get_metadata_stmt, 3);
    reaction_network->factor_duplicate = sqlite3_column_double(get_metadata_stmt, 4);


    reaction_network->dependency_threshold = dependency_threshold;

    // allocate number of reactants array
    reaction_network->number_of_reactants = calloc(
        reaction_network->number_of_reactions, sizeof(int));

    // allocate reactant array
    int *reactants_values = calloc(
        2 * reaction_network->number_of_reactions, sizeof(int));

    reaction_network->reactants = calloc(
        reaction_network->number_of_reactions, sizeof(int *));

    for (i = 0; i < reaction_network->number_of_reactions; i++) {
      reaction_network->reactants[i] = reactants_values + 2 * i;
    }

    // allocate number of products array
    reaction_network->number_of_products = calloc(
        reaction_network->number_of_reactions, sizeof(int));

    // allocate products array
    int *products_values = calloc(
        2 * reaction_network->number_of_reactions, sizeof(int));

    reaction_network->products = calloc(
        reaction_network->number_of_reactions, sizeof(int *));

    for (i = 0; i < reaction_network->number_of_reactions; i++) {
      reaction_network->products[i] = products_values + 2 * i;
    }

    //allocate rates array
    reaction_network->rates = calloc(
        reaction_network->number_of_reactions, sizeof(double));


    rc = sqlite3_prepare_v2(
        reaction_network_database,
        sql_get_reactions,
        -1,
        &get_reactions_stmt,
        NULL);

    if (rc != SQLITE_OK) {
        printf("new_reaction_network error %s\n",
               sqlite3_errmsg(reaction_network_database));
        return NULL;
    }

    // reading reactions from db
    for (i = 0; i < reaction_network->number_of_reactions; i++) {
        sqlite3_step(get_reactions_stmt);
        reaction_index = sqlite3_column_int(get_reactions_stmt,0);

        reaction_network->number_of_reactants[reaction_index] =
            sqlite3_column_int(get_reactions_stmt,1);

        reaction_network->number_of_products[reaction_index] =
            sqlite3_column_int(get_reactions_stmt,2);

        reaction_network->reactants[reaction_index][0] =
            sqlite3_column_int(get_reactions_stmt,3);

        reaction_network->reactants[reaction_index][1] =
            sqlite3_column_int(get_reactions_stmt,4);

        reaction_network->products[reaction_index][0] =
            sqlite3_column_int(get_reactions_stmt,5);

        reaction_network->products[reaction_index][1] =
            sqlite3_column_int(get_reactions_stmt,6);

        reaction_network->rates[reaction_index] =
            sqlite3_column_double(get_reactions_stmt,7);
    }


    // allocate initial state
    reaction_network->initial_state = calloc(
        reaction_network->number_of_species, sizeof(int));

    rc = sqlite3_prepare_v2(
        initial_state_database,
        sql_get_initial_state,
        -1,
        &get_initial_state_stmt,
        NULL);

    if (rc != SQLITE_OK) {
        printf("new_reaction_network error %s\n",
               sqlite3_errmsg(initial_state_database));
        return NULL;
    }

    // fill initial state
    for (i = 0; i < reaction_network->number_of_species; i++) {
        sqlite3_step(get_initial_state_stmt);
        species_index = sqlite3_column_int(get_initial_state_stmt,0);
        reaction_network->initial_state[species_index] =
            sqlite3_column_int(get_initial_state_stmt,1);
    }


    initialize_dependency_graph(reaction_network);
    initialize_propensities(reaction_network);



    sqlite3_finalize(get_metadata_stmt);
    sqlite3_finalize(get_reactions_stmt);
    sqlite3_finalize(get_initial_state_stmt);
    return reaction_network;

}


void free_reaction_network(ReactionNetwork *reaction_network) {
    free(reaction_network->number_of_reactants);
    free(reaction_network->reactants[0]);
    free(reaction_network->reactants);
    free(reaction_network->number_of_products);
    free(reaction_network->products[0]);
    free(reaction_network->products);
    free(reaction_network->rates);
    free(reaction_network->initial_state);
    free(reaction_network->initial_propensities);

    int i; // reaction index
    for (i = 0; i < reaction_network->number_of_reactions; i++)
        free_dependents_node(reaction_network->dependency_graph + i);

    free(reaction_network->dependency_graph);

    free(reaction_network);

}

DependentsNode *get_dependency_node(ReactionNetwork *reaction_network, int index) {
    DependentsNode *node = reaction_network->dependency_graph + index;

    pthread_mutex_lock(&node->mutex);

    // if we haven't computed this node
    // and the reaction has been seen more times than the threshold:
    // compute it
    if ( ! node->dependents &&
         node->number_of_occurrences >= reaction_network->dependency_threshold )
        compute_dependency_node(reaction_network, index);

    node->number_of_occurrences++;
    pthread_mutex_unlock(&node->mutex);
    return node;
}


void compute_dependency_node(ReactionNetwork *reaction_network, int index) {

    DependentsNode *node = reaction_network->dependency_graph + index;

    int number_of_dependents_count = 0;
    int j; // reaction index
    int l, m, n; // reactant and product indices

    for (j = 0; j < reaction_network->number_of_reactions; j++) {
        bool flag = false;

        for (l = 0; l < reaction_network->number_of_reactants[j]; l++) {
            for (m = 0; m < reaction_network->number_of_reactants[index]; m++) {
                if (reaction_network->reactants[j][l] ==
                    reaction_network->reactants[index][m])
                    flag = true;
            }

            for (n = 0; n < reaction_network->number_of_products[index]; n++) {
                if (reaction_network->reactants[j][l] ==
                    reaction_network->products[index][n])
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
        for (l = 0;
             l < reaction_network->number_of_reactants[current_reaction];
             l++) {
            for (m = 0; m < reaction_network->number_of_reactants[index]; m++) {
                if (reaction_network->reactants[current_reaction][l] ==
                    reaction_network->reactants[index][m])
                    flag = true;
            }

            for (n = 0; n < reaction_network->number_of_products[index]; n++) {
                if (reaction_network->reactants[current_reaction][l] ==
                    reaction_network->products[index][n])
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

void initialize_dependency_graph(ReactionNetwork *reaction_network) {


    int i; // reaction index
    reaction_network->dependency_graph = calloc(
        reaction_network->number_of_reactions,
        sizeof(DependentsNode)
        );

    for (i = 0; i < reaction_network->number_of_reactions; i++) {
        initialize_dependents_node(reaction_network->dependency_graph + i);
    }

    // this is how you would precompute the dependency graph.
    // Doing so is completely inpractical for large networks.
    // for (i = 0; i < rnp->number_of_reactions; i++) {
    //   compute_dependency_node(rnp, i);
    // }

}

double compute_propensity(ReactionNetwork *reaction_network,
                         int *state,
                         int reaction) {
    double p;
    // zero reactants
    if (reaction_network->number_of_reactants[reaction] == 0)
        p = reaction_network->factor_zero
            * reaction_network->rates[reaction];

    // one reactant
    else if (reaction_network->number_of_reactants[reaction] == 1)
        p = state[reaction_network->reactants[reaction][0]]
            * reaction_network->rates[reaction];


    // two reactants
    else {
        if (reaction_network->reactants[reaction][0] ==
            reaction_network->reactants[reaction][1])
            p = reaction_network->factor_duplicate
                * reaction_network->factor_two
                * state[reaction_network->reactants[reaction][0]]
                * (state[reaction_network->reactants[reaction][0]] - 1)
                * reaction_network->rates[reaction];

        else
            p = reaction_network->factor_two
                * state[reaction_network->reactants[reaction][0]]
                * state[reaction_network->reactants[reaction][1]]
                * reaction_network->rates[reaction];

    }

    return p;
}

void initialize_propensities(ReactionNetwork *reaction_network) {
    reaction_network->initial_propensities = calloc(
        reaction_network->number_of_reactions, sizeof(double));
    for (int reaction = 0;
         reaction < reaction_network->number_of_reactions;
         reaction++) {
        reaction_network->initial_propensities[reaction] =
            compute_propensity(reaction_network,
                               reaction_network->initial_state, reaction);
    }
}
