# RNMC

RNMC is a program for simulating reaction networks heavily inspired by [SPPARKS](https://spparks.sandia.gov/). RNMC is designed to run large numbers of simulations of a fixed system (fixed reaction network and fixed initial state) in parallel.

### Dependencies

RNMC depends on [GSL](https://www.gnu.org/software/gsl/) for pseudo random number generation and [sqlite](https://www.sqlite.org/index.html) for the database interface. Make sure that `CPATH`, `LIBRARY_PATH` and `LD_LIBRARY_PATH` are set appropriately if you aren't using versions bundled with your system.

### Building

RNMC_native is built using meson and ninja:
```
CC=clang meson setup --buildtype=debug build
cd build
ninja
meson test
```

### reaction_network_dir

Inorder to remain indipendent of any particular language wrapper, RNMC ingests reaction networks from a series of files. A folder containing these files is the first argument to `RNMC`
- `number_of_reactions`:
- `number_of_species`:
- `number_of_products`: number of products of each reaction
- `products`: products of each reaction
- `number_of_reactants`: number of reactants of each reaction
- `reactants`: reactants of each reaction
- `rates`: rates for each reaction
- `initial_state`: initial state of the simulation.
- `factor_zero`: rate modifier for reactions of the form (nothing) -> ...
- `factor_two`: rate modifier for reactions of the form A + B -> ...
- `factor_duplicate`: rate modifier for reactions of the form A + A -> ...

See `./test_materials/ronalds_network` for an example (thanks Ronald!)

Alternatively, the first 7 of these files can be wrapped up into a sqlite database with file name `rn.sqlite`. This is useful when dealing with reaction networks with hundreds of millions of reactions. An example is contained in `./test_materials/ronalds_network_db`. The reactions table has a column `reaction_string`. This column has an index which allows us to efficiently identify duplicate reactions. Because of this index, if the reaction table grows too big, insertions grind to a halt, so we shard the reactions table as demonstrated in the example database, which has shard size 20. In practice, you want to set shard size somewhere between 1 million and 5 million.


### simulation_params

folder containing the following files:

- `number_of_seeds`:
- `number_of_threads`:
- `seeds`: list of seeds. `0` is not considered a valid seed, and if a thread receives zero, it will terminate.
- `time_cutoff`: how long to run the simulations (in simulation time)
- `step_cutoff`: how many steps to run the simulation

Note: Only one of `time_cutoff` or `step_cutoff` is needed. if `step_cutoff` is present, it will be used over `time_cutoff`

See `./test_materials/simulation_params` for an example.
