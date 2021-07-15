# RNMC

RNMC is a program for simulating reaction networks heavily inspired by [SPPARKS](https://spparks.sandia.gov/). RNMC is designed to run large numbers of simulations of a fixed system (fixed reaction network and fixed initial state) in parallel.

### Dependencies

RNMC depends on [GSL](https://www.gnu.org/software/gsl/) for pseudo random number generation and [sqlite](https://www.sqlite.org/index.html) for the database interface. Make sure that `CPATH`, `LIBRARY_PATH` and `LD_LIBRARY_PATH` are set appropriately if you aren't using versions bundled with your system.

### Building

RNMC is built using meson and ninja:

```
CC=clang meson setup --buildtype=debug build
cd build
ninja
```

### Testing

Run the test using `test.sh` from the root directory of the repository.

### Running

RNMC is run as follows:

```
RNMC --database=rn.sqlite --number_of_simulations=1000 --base_seed=1000 --thread_count=8 --step_cutoff=200
```

- `database`: a sqlite database containing the reaction network, initial state and metadata. The simulation trajectories are also written into the database
- `number_of_simulation`: an integer specifying how many simulations to run
- `base_seed`: seeds used are `base_seed, base_seed+1, ..., base_seed+number_of_simulations-1`
- `thread_count`: is how many threads to use.
- `step_cutoff`: how many steps in each simulation
- `logging`: whether to print logging info to stdout

### The Reaction Network Database

There should be 4 tales in the reaction network database:
```
    CREATE TABLE metadata (
            number_of_species   INTEGER NOT NULL,
            number_of_reactions INTEGER NOT NULL,
            factor_zero         REAL NOT NULL,
            factor_two          REAL NOT NULL,
            factor_duplicate    REAL NOT NULL
    );
```
the factors can be used to modify rates of reactions which have zero or two reactants, or have duplicate reactants.

```
    CREATE TABLE reactions (
            reaction_id         INTEGER NOT NULL PRIMARY KEY,
            number_of_reactants INTEGER NOT NULL,
            number_of_products  INTEGER NOT NULL,
            reactant_1          INTEGER NOT NULL,
            reactant_2          INTEGER NOT NULL,
            product_1           INTEGER NOT NULL,
            product_2           INTEGER NOT NULL,
            rate                REAL NOT NULL
    );

```

```
    CREATE TABLE trajectories (
            seed         INTEGER NOT NULL,
            step         INTEGER NOT NULL,
            reaction_id  INTEGER NOT NULL,
            time         REAL NOT NULL
    );
```

```
    CREATE TABLE initial_state (
            species_id             INTEGER NOT NULL PRIMARY KEY,
            count                  INTEGER NOT NULL
    );

```
