#include "solvers.h"
#include "sampler.h"
#include "reaction_network.h"
#include "simulation.h"
#include "dispatcher.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"


bool test_samplers();
bool test_serialization();
bool run_simulation();
bool run_test_dispatcher();
bool test_long_simulation_history();
bool run_tests();
