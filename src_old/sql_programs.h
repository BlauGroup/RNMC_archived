#ifndef SQL_PROGRAMS_H
#define SQL_PROGRAMS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sqlite3.h>
#include "reaction_network.h"

static char reaction_network_db_postix[] = "/rn.sqlite";

static char create_metadata_table_sql[] =
  "CREATE TABLE metadata ("
  "        number_of_species   INTEGER NOT NULL,"
  "        number_of_reactions INTEGER NOT NULL,"
  "        shard_size          INTEGER NOT NULL,"
  "        number_of_shards    INTEGER NOT NULL"
  ");";

static char insert_metadata_sql[] =
  "INSERT INTO metadata ("
  "        number_of_species,"
  "        number_of_reactions,"
  "        shard_size,"
  "         number_of_shards) "
  "VALUES (?1, ?2, ?3, ?4);";

static char get_metadata_sql[] =
  "SELECT * FROM metadata;";


// because of sharding, these sql programs need to be dynamically built.
// they get generated when toDatabaseSQL and fromDatabaseSQL get initialized
char *create_reactions_table_sql(int shard);
char *insert_reaction_sql(int shard);
char *get_reaction_sql(int shard);


// object to manage state required to serialize a reaction network to db
typedef struct toDatabaseSQL {
  int number_of_shards;
  int shard_size;
  sqlite3 *db;
  char **create_reactions_table;
  char **insert_reaction;
  char *create_metadata_table;
  char *insert_metadata;
  sqlite3_stmt **insert_reaction_stmt;
  sqlite3_stmt *insert_metadata_stmt;
} ToDatabaseSQL;


ToDatabaseSQL *new_to_database_sql(int number_of_shards,
                                   int shard_size,
                                   char *directory);

void free_to_database_sql(ToDatabaseSQL *p);
void insert_metadata(ToDatabaseSQL *p,
                    int number_of_species,
                    int number_of_reactions,
                    int shard_size,
                    int number_of_shards);

void insert_reaction(ToDatabaseSQL *p,
                     int reaction_id,
                     char *reaction_string,
                     int number_of_reactants,
                     int number_of_products,
                     int reactant_1,
                     int reactant_2,
                     int product_1,
                     int product_2,
                     double rate);


// object to manage state required to build a reaction network from db
typedef struct fromDatabaseSQL {
  int number_of_shards;
  int shard_size;
  int number_of_species;
  int number_of_reactions;
  sqlite3 *db;
  char *get_metadata;
  char **get_reaction;
  sqlite3_stmt **get_reaction_stmt;
  sqlite3_stmt *get_metadata_stmt;

} FromDatabaseSQL;



FromDatabaseSQL *new_from_database_sql(char *directory);

void free_from_database_sql(FromDatabaseSQL *p);


void get_reaction(FromDatabaseSQL *p,
                  int shard,
                  ReactionNetwork *rnp
                  );

#endif
