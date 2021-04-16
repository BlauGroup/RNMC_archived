#ifndef SQL_PROGRAMS_H
#define SQL_PROGRAMS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sqlite3.h>

static char reaction_network_db_postix[] = "/rn.sqlite";

static char create_metadata_table_sql[] =
  "CREATE TABLE metadata ("
  "        number_of_species   INTEGER NOT NULL,"
  "        number_of_reactions INTEGER NOT NULL,"
  "        shard_size          INTEGER NOT NULL"
  ");";

static char insert_metadata_sql[] =
  "INSERT INTO metadata ("
  "        number_of_species,"
  "        number_of_reactions,"
  "        shard_size)"
  "VALUES (?1, ?2, ?3);";

static char get_metadata_sql[] =
  "SELECT * FROM metadata;";


char *create_reactions_table_sql(int shard);
char *insert_reaction_sql(int shard);
char *get_reaction_sql(int shard);


// if reactants and products don't exist, we use the value -1

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
int create_tables(ToDatabaseSQL *p);
void insert_metadata(ToDatabaseSQL *p,
                    int number_of_species,
                    int number_of_reactions,
                    int shard_size);

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


typedef struct fromDatabaseSQL {
  int number_of_shards;
  int shard_size;
  sqlite3 *db;
  char *get_metadata;
  char **get_reaction;
  sqlite3_stmt **get_reaction_stmt;
  sqlite3_stmt *get_metadata_stmt;

} FromDatabaseSQL;



FromDatabaseSQL *new_from_database_sql(int number_of_shards);
void free_from_database_sql(FromDatabaseSQL *p);
void get_metadata(FromDatabaseSQL *p,
                  int *number_of_speciesp,
                  int *number_of_reactionsp);


void get_reaction(FromDatabaseSQL *p,
                  int reaction_id,
                  int *number_of_reactantsp,
                  int *number_of_productsp,
                  int *reactant_1p,
                  int *reactant_2p,
                  int *product_1p,
                  int *product_2p,
                  double *rate
                  );

#endif
