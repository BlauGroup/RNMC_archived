#ifndef SQL_PROGRAMS_H
#define SQL_PROGRAMS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char create_metadata_table[] = "CREATE TABLE metadata ("
                               "        number_of_species   INTEGER NOT NULL,"
                               "        number_of_reactions INTEGER NOT NULL,"
                               "        shard_size          INTEGER NOT NULL"
                               ");";

char insert_metadata[] =
  "INSERT INTO metadata ("
  "        number_of_species,"
  "        number_of_reactions,"
  "        shard_size)"
  "VALUES (?1, ?2, ?3);";

char get_metadata[] =
  "SELECT * FROM metadata;";


char *create_reactions_table(int shard);


// if reactants and products don't exist, we use the value -1

typedef struct toDatabaseSQL {
  int number_of_shards;
  char **create_reactions_table;
  char **insert_reaction;
} ToDatabaseSQL;

typedef struct fromDatabaseSQL {
  int number_of_shards;
  char **get_reaction;
} FromDatabaseSQL;


ToDatabaseSQL *new_to_database_sql(int number_of_shards);
void free_to_database_sql(ToDatabaseSQL *p);

FromDatabaseSQL *new_from_database_sql(int number_of_shards);
void free_from_database_sql(FromDatabaseSQL *p);






char insert_reaction[] =
  "INSERT INTO reactions ("
  "        reaction_id,"
  "        reaction_string,"
  "        number_of_reactants,"
  "        number_of_products,"
  "        reactant_1,"
  "        reactant_2,"
  "        product_1,"
  "        product_2,"
  "        rate) "
  "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);";


char get_reaction[] =
  "SELECT reaction_id,"
  "       number_of_reactants,"
  "       number_of_products,"
  "       reactant_1,"
  "       reactant_2,"
  "       product_1,"
  "       product_2,"
  "       rate "
  "FROM reactions;";

#endif
