#include "test.h"

bool test_samplers() {
  bool flag = true;
  double inital_propensities[] = {.1, .1, .8, .1, .4, .1, .0};
  Solve *p = new_solve(linear, 42, 7, inital_propensities);
  Solve *q = new_solve(tree, 42, 7, inital_propensities);
  double dt = 0;
  bool result = true;

  for (int i = 0; i < 1000; i++) {
    int p_sample = p->event(p, &dt);
    int q_sample = q->event(q, &dt);
    if (p_sample != q_sample) {
      result = false;
      break;
    }
  }

  double wide_propensities[] = {1e-300, 1e300, 1e300};

  Solve *r = new_solve(linear, 42, 3, wide_propensities);
  Solve *s = new_solve(tree, 42, 3, wide_propensities);
  for (int i = 0; i < 1000; i++) {
    int r_sample = r->event(p, &dt);
    int s_sample = s->event(q, &dt);
    if (r_sample != s_sample) {
      result = false;
      break;
    }
  }

  free_solve(p);
  free_solve(q);
  free_solve(r);
  free_solve(s);

  if (result)
    puts(ANSI_COLOR_GREEN "passed: sampler test" ANSI_COLOR_RESET);
  else {
    puts(ANSI_COLOR_RED "failed: sampler test" ANSI_COLOR_RESET);
    flag = false;
  }


  return flag;
}


// currently segfaults if not run from the correct directory
bool test_serialization() {
  bool flag = true;
  char *ronalds_network_dir = "./ronalds_network";
  char *test_network_dir = "./test_network";
  char *test_network_with_db_dir = "./test_network_db";
  char *test_network_with_db_shard_dir = "./test_network_db_shard";

  ReactionNetwork *rnp = new_reaction_network_from_files(ronalds_network_dir, false);
  reaction_network_to_files(rnp, test_network_dir);
  char *cmd = "diff -r --exclude=simulation_histories ./ronalds_network ./test_network > /dev/null";
  int status = system(cmd);
  // status code 0 means no differences
  // non zero status code means difference found
  if (status) {
    puts(ANSI_COLOR_RED "failed: file to ReactionNetwork to file test" ANSI_COLOR_RESET);
    flag = false;
  } else
    puts(ANSI_COLOR_GREEN "passed: file to ReactionNetwork to file test" ANSI_COLOR_RESET);

  ReactionNetwork *rnp_copy = new_reaction_network_from_files(test_network_dir, false);

  if (reaction_networks_differ(rnp, rnp_copy)) {
    puts(ANSI_COLOR_RED "failed: ReactionNetwork to file to ReactionNetwork test" ANSI_COLOR_RESET);
    flag = false;
  } else {
    puts(ANSI_COLOR_GREEN "passed: ReactionNetwork to file to ReactionNetwork test" ANSI_COLOR_RESET);
  }


  reaction_network_to_db(rnp, test_network_with_db_dir, 1000000);
  ReactionNetwork *rnp_from_db = new_reaction_network_from_db(test_network_with_db_dir, false);
  if (reaction_networks_differ(rnp, rnp_from_db)) {
    puts(ANSI_COLOR_RED "failed: ReactionNetwork to single shard db to ReactionNetwork test" ANSI_COLOR_RESET);
    flag = false;
  } else {
    puts(ANSI_COLOR_GREEN "passed: ReactionNetwork to single shard db to ReactionNetwork test" ANSI_COLOR_RESET);
  }

  reaction_network_to_db(rnp, test_network_with_db_shard_dir, 20);
  ReactionNetwork *rnp_from_db_shard = new_reaction_network_from_db(test_network_with_db_dir, false);
  if (reaction_networks_differ(rnp, rnp_from_db_shard)) {
    puts(ANSI_COLOR_RED "failed: ReactionNetwork to multi shard db to ReactionNetwork test" ANSI_COLOR_RESET);
    flag = false;
  } else {
    puts(ANSI_COLOR_GREEN "passed: ReactionNetwork to multi shard db to ReactionNetwork test" ANSI_COLOR_RESET);
  }


  free_reaction_network(rnp);
  free_reaction_network(rnp_copy);
  free_reaction_network(rnp_from_db);
  free_reaction_network(rnp_from_db_shard);
  status = system("rm -r ./test_network");
  status = system("rm -r ./test_network_db");
  status = system("rm -r ./test_network_db_shard");
  return flag;
}


bool run_test_simulation() {
  bool flag = true;
  char *ronalds_network_dir = "./ronalds_network";
  ReactionNetwork *rnp = new_reaction_network_from_files(ronalds_network_dir, false);
  Simulation *sp = new_simulation(rnp, 42, tree);
  run_until(sp, 5);
  simulation_history_to_file(sp);
  char *reaction_cmd = "diff ./test_simulation_histories/reactions_42 ./ronalds_network/simulation_histories/reactions_42 > /dev/null";
  int reaction_status = system(reaction_cmd);
  // status code 0 means no differences
  // non zero status code means difference found
  if (reaction_status) {
    puts(ANSI_COLOR_RED "failed: seed 42 simulation reactions have changed" ANSI_COLOR_RESET);
    flag = false;
  } else
    puts(ANSI_COLOR_GREEN "passed: seed 42 simulation reactions normal" ANSI_COLOR_RESET);

  char *time_cmd = "diff ./test_simulation_histories/times_42 ./ronalds_network/simulation_histories/times_42 > /dev/null";
  int time_status = system(time_cmd);

  if (time_status) {
    puts(ANSI_COLOR_RED "failed: seed 42 simulation times have changed" ANSI_COLOR_RESET);
    flag = false;
  } else
    puts(ANSI_COLOR_GREEN "passed: seed 42 simulation times normal" ANSI_COLOR_RESET);

  if (sp->step == simulation_history_length(sp->history))
        puts(ANSI_COLOR_GREEN "passed: history length agrees with number of steps" ANSI_COLOR_RESET);
  else {
    puts(ANSI_COLOR_RED "failed: history length is different to number of steps" ANSI_COLOR_RESET);
    flag = false;
  }

  free_simulation(sp);
  free_reaction_network(rnp);
  return flag;

}

bool run_test_dispatcher() {
  bool flag = true;
  Dispatcher *dp = new_dispatcher("./ronalds_network","./simulation_params", false);
    if (!dp) {
      puts(ANSI_COLOR_RED
           "failed: dispatcher wasn't created"
           ANSI_COLOR_RESET);
      exit(1);
    }

  run_dispatcher(dp);
  free_dispatcher(dp);

  char *cmd = "diff -r ./test_simulation_histories ./ronalds_network/simulation_histories > /dev/null";
  int status = system(cmd);
  // status code 0 means no differences
  // non zero status code means difference found
  if (status) {
    puts(ANSI_COLOR_RED "failed: dispatcher produced different result" ANSI_COLOR_RESET);
    flag = false;
  } else
    puts(ANSI_COLOR_GREEN "passed: dispatcher behaving correctly" ANSI_COLOR_RESET);

  return flag;

}

bool test_long_simulation_history() {
  bool flag = true;
  double inital_propensities[] = {.1, .1, .8, .1, .4, .1, .0};
  int length = 42069;
  int values[42069];
  double times[42069];
  double dt;
  double time = 0.0;
  int sample;
  int i, j;
  Solve *q = new_solve(tree, 42, 7, inital_propensities);
  SimulationHistory *sh = new_simulation_history();

  for (i = 0; i < length; i++) {
    sample = q->event(q, &dt);
    values[i] = sample;
    time += dt;
    times[i] = time;
    insert_history_element(sh, sample, time);
  }

  i = 0;
  bool result = true;
  Chunk *chunk = sh->first_chunk;
  while (chunk) {
    for (j = 0; j < chunk->next_free_index; j++) {
      if (chunk->data[j].reaction != values[i] || chunk->data[j].time != times[i]) {
        result = false;
      }
      i++;
    }
    chunk = chunk->next_chunk;
  }

  if (result)
    puts(ANSI_COLOR_GREEN "passed: history readback correct" ANSI_COLOR_RESET);
  else {
    puts(ANSI_COLOR_RED "failed: history readback incorrect" ANSI_COLOR_RESET);
    flag = false;
  }

  free_solve(q);
  free_simulation_history(sh);

  return flag;
}

bool run_tests(char *test_materials_dir) {
  int return_code = chdir(test_materials_dir);
  bool result = test_samplers() &&
    test_serialization() &&
    run_test_simulation() &&
    test_long_simulation_history() &&
    run_test_dispatcher();

  return result;
}


int main(int argc, char **argv) {
  bool flag = false;
  if (argc != 2)
    puts("usage: ./test test_materials_directory");
  else
    flag = run_tests(argv[1]);

  if (flag)
    return 0;
  else
    return 1;
}
