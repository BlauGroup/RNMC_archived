#include "dispatcher.h"

int main(int argc, char **argv) {

    int rc = system("cp /home/danielbarter/RNMC_native/test_materials/rn.sqlite /home/danielbarter/RNMC_native/test_materials/rn.sqlite_copy" );

    Dispatcher *dp = new_dispatcher(
        "/home/danielbarter/RNMC_native/test_materials/rn.sqlite_copy",
        1000,
        1000,
        4,
        200,
        true);

    run_dispatcher(dp);

    free_dispatcher(dp);
}
