#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include "dispatcher.h"

void print_usage() {
    puts(
        "Usage: specify the following options\n"
        "--database\n"
        "--number_of_simulations\n"
        "--base_seed\n"
        "--thread_count\n"
        "--step_cutoff\n"
        );
}

int main(int argc, char **argv) {

    if (argc != 6) {
        print_usage();
        exit(EXIT_FAILURE);
    }


    struct option long_options[] = {
        {"database", required_argument, NULL, 'd'},
        {"number_of_simulations", required_argument, NULL, 'n'},
        {"base_seed", required_argument, NULL, 'b'},
        {"thread_count", required_argument, NULL, 't'},
        {"step_cutoff", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0}
        // last element of options array needs to be filled with zeros
    };

    int c;
    int option_index = 0;

    char *database;
    int number_of_simulations;
    int base_seed;
    int thread_count;
    int step_cutoff;

    while ((c = getopt_long_only(
                argc, argv, "",
                long_options,
                &option_index)) != -1) {

        switch (c) {

        case 'd':
            database = optarg;
            break;

        case 'n':
            number_of_simulations = atoi(optarg);
            break;

        case 'b':
            base_seed = atoi(optarg);
            break;

        case 't':
            thread_count = atoi(optarg);
            break;

        case 's':
            step_cutoff = atoi(optarg);
            break;

        default:
            // if an unexpected argument is passed, exit
            print_usage();
            exit(EXIT_FAILURE);

        }

    }

    Dispatcher *dispatcher = new_dispatcher(
        database,
        number_of_simulations,
        base_seed,
        thread_count,
        step_cutoff,
        true
        );

    if (!dispatcher) {
      puts("dispatcher wasn't created");
      exit(EXIT_FAILURE);
    }
    run_dispatcher(dispatcher);
    free_dispatcher(dispatcher);

    exit(EXIT_SUCCESS);

}
