#include "dispatcher.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    puts("usage: ./RNMC reaction_network_dir simulation_params");
    exit(0);
  }

  else {
    Dispatcher *dp = new_dispatcher(argv[1],
                                    argv[2],
                                    true);
    if (!dp) {
      puts("dispatcher wasn't created");
      exit(1);
    }
    run_dispatcher(dp);
    free_dispatcher(dp);

    exit(0);
  }

}
