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
        "--gc_interval\n"
        );
}

int main(int argc, char **argv) {

    if (argc != 8) {
        print_usage();
        exit(EXIT_FAILURE);
    }


    struct option long_options[] = {
        {"database", required_argument, NULL, 1},
        {"number_of_simulations", required_argument, NULL, 2},
        {"base_seed", required_argument, NULL, 3},
        {"thread_count", required_argument, NULL, 4},
        {"step_cutoff", required_argument, NULL, 5},
        {"gc_interval", required_argument, NULL, 6},
        {"gc_threshold", required_argument, NULL, 7},
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
    long int gc_interval;
    int gc_threshold;
    bool logging = true;

    while ((c = getopt_long_only(
                argc, argv, "",
                long_options,
                &option_index)) != -1) {

        switch (c) {

        case 1:
            database = optarg;
            break;

        case 2:
            number_of_simulations = atoi(optarg);
            break;

        case 3:
            base_seed = atoi(optarg);
            break;

        case 4:
            thread_count = atoi(optarg);
            break;

        case 5:
            step_cutoff = atoi(optarg);
            break;

        case 6:
            gc_interval = atoi(optarg);
            break;

        case 7:
            gc_threshold = atoi(optarg);
            break;



        default:
            // if an unexpected argument is passed, exit
            print_usage();
            exit(EXIT_FAILURE);
            break;

        }

    }

    Dispatcher *dispatcher = new_dispatcher(
        database,
        number_of_simulations,
        base_seed,
        thread_count,
        step_cutoff,
        gc_interval,
        gc_threshold,
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
