# RNMC

RNMC is a program for simulating reaction networks heavily inspired by [SPPARKS](https://spparks.sandia.gov/). RNMC is designed to run large numbers of simulations of a fixed system (fixed reaction network and fixed initial state) in parallel. Unlike SPPARKS, RNMC can simulate densely connected reaction networks because it computes the reaction dependency graph dynamically and it is shared between all simulation threads.

### Dependencies

RNMC depends on [GSL](https://www.gnu.org/software/gsl/) for pseudo random number generation and [sqlite](https://www.sqlite.org/index.html) for the database interface.

### Building

On a machine with system versions of GSL and sqlite, the `RNMC` executable can be built like this:

```
CC=gcc ./build.sh
```

Note that the build script uses the `gsl-config` utility to find headers and libraries for GSL. If you are on a cluster and sqlite is not present, it can be built as follows:

```
cd $HOME
wget https://www.sqlite.org/2021/sqlite-amalgamation-3360000.zip
unzip sqlite-amalgamation-3360000.zip
cd sqlite-amalgamation-3360000
gcc -o libsqlite3.so -shared -fPIC sqlite3.c -lpthread -ldl
```

in which case `RNMC` can be built like this:

```
export CPATH=$HOME/sqlite-amalgamation-3360000:$CPATH
export LIBRARY_PATH=$HOME/sqlite-amalgamation-3360000:$LIBRARY_PATH
CC=gcc ./build.sh
```

### Testing

Run the test using `test.sh` from the root directory of the repository.

### Running

RNMC is run as follows:

```
RNMC --reaction_database=rn.sqlite --initial_state_database=initial_state.sqlite --number_of_simulations=1000 --base_seed=1000 --thread_count=8 --step_cutoff=200 --gc_interval=10 --gc_threshold=1
```

- `reaction_database`: a sqlite database containing the reaction network and metadata.
- `initial_state_database` : a sqlite database containing initial state. The simulation trajectories are also written into the database
- `number_of_simulation`: an integer specifying how many simulations to run
- `base_seed`: seeds used are `base_seed, base_seed+1, ..., base_seed+number_of_simulations-1`
- `thread_count`: is how many threads to use.
- `step_cutoff`: how many steps in each simulation
- `dependency_threshold`: if simulations run for a long time, the dependency graph can grow quite large. We slow down its growth by only computing the dependency node corresponding to a reaction after it has been seen `dependency_threshold` times. Set to zero if you want to compute dependents on first occurrence. 

### The Reaction Network Database

There should be 2 tables in the reaction network database:
```
    CREATE TABLE metadata (
            number_of_species   INTEGER NOT NULL,
            number_of_reactions INTEGER NOT NULL
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
There are 3 tables in the initial state database:
```
    CREATE TABLE trajectories (
            seed         INTEGER NOT NULL,
            step         INTEGER NOT NULL,
            reaction_id  INTEGER NOT NULL,
            time         REAL NOT NULL
    );
```

```
    CREATE TABLE factors (
            factor_zero         REAL NOT NULL,
            factor_two          REAL NOT NULL,
            factor_duplicate    REAL NOT NULL)
```

```
    CREATE TABLE initial_state (
            species_id             INTEGER NOT NULL PRIMARY KEY,
            count                  INTEGER NOT NULL
    );

```
