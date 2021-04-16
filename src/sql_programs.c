#include "sql_programs.h"

char *create_reactions_table_sql(int shard) {
  char *sql_program = malloc(sizeof(char) * 2048);
  char shard_string[256];
  char *end;

  sprintf(sql_program, "%d", shard);

  end = stpcpy(sql_program, "CREATE TABLE reactions_");
  end = stpcpy(end, shard_string);
  end = stpcpy(end, "(\n");
  end = stpcpy(end, "reaction_id INTEGER NOT NULL PRIMARY KEY,\n");
  end = stpcpy(end, "reaction_string TEXT NOT NULL,\n");
  end = stpcpy(end, "number_of_reactants INTEGER NOT NULL,\n");
  end = stpcpy(end, "number_of_products  INTEGER NOT NULL,\n");
  end = stpcpy(end, "reactant_1 INTEGER NOT NULL,\n");
  end = stpcpy(end, "reactant_2 INTEGER NOT NULL,\n");
  end = stpcpy(end, "product_1 INTEGER NOT NULL,\n");
  end = stpcpy(end, "product_2 INTEGER NOT NULL,\n");
  end = stpcpy(end, "rate REAL NOT NULL);\n\n");

  end = stpcpy(end, "CREATE UNIQUE INDEX reaction_");
  end = stpcpy(end, shard_string);
  end = stpcpy(end, "_string_idx ON reactions_");
  end = stpcpy(end, shard_string);
  end = stpcpy(end, " (reaction_string);");
  return sql_program;
}

char *insert_reaction_sql(int shard) {
  char *sql_program = malloc(sizeof(char) * 2048);
  char shard_string[256];
  char *end;

  sprintf(sql_program, "%d", shard);

  end = stpcpy(sql_program, "INSERT INTO reactions_");
  end = stpcpy(end, shard_string);
  end = stpcpy(end, " (\n");
  end = stpcpy(end, "reaction_id,\n");
  end = stpcpy(end, "reaction_string,\n");
  end = stpcpy(end, "number_of_reactants,\n");
  end = stpcpy(end, "number_of_products,\n");
  end = stpcpy(end, "reactant_1,\n");
  end = stpcpy(end, "reactant_2,\n");
  end = stpcpy(end, "product_1,\n");
  end = stpcpy(end, "product_2,\n");
  end = stpcpy(end, "rate)\n");
  end = stpcpy(end, "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);");

  return sql_program;
}

char *get_reaction_sql(int shard) {
  char *sql_program = malloc(sizeof(char) * 2048);
  char shard_string[256];
  char *end;

  sprintf(sql_program, "%d", shard);

  end = stpcpy(sql_program, "SELECT reaction_id,\n");
  end = stpcpy(end, "number_of_reactants,\n");
  end = stpcpy(end, "number_of_products,\n");
  end = stpcpy(end, "reactant_1,\n");
  end = stpcpy(end, "reactant_2,\n");
  end = stpcpy(end, "product_1,\n");
  end = stpcpy(end, "product_2,\n");
  end = stpcpy(end, "rate \n");
  end = stpcpy(end, "FROM reactions_");
  end = stpcpy(end, shard_string);
  end = stpcpy(end, ";");

  return sql_program;
}

ToDatabaseSQL *new_to_database_sql(int number_of_shards, int shard_size, char *directory) {

  char *end;
  char path[2048];
  ToDatabaseSQL *p = malloc(sizeof(ToDatabaseSQL));


  if (strlen(directory) > 1024) {
    puts("new_to_database_sql: directory path too long");
    return NULL;
  }

  DIR *dir = opendir(directory);
  if (dir) {
    puts("new_to_database_sql: directory already exists");
    closedir(dir);
    return NULL;
  }

  if (mkdir(directory, 0777)) {
    puts("new_to_database_sql: failed to make directory");
    return NULL;
  }

  end = stpcpy(path, directory);
  stpcpy(end, reaction_network_db_postix);


  // TODO: check error code here
  p->number_of_shards = number_of_shards;
  p->shard_size = shard_size;
  sqlite3_open(path, &p->db);
  p->create_reactions_table = malloc(sizeof(char *) * number_of_shards);
  p->insert_reaction = malloc(sizeof(char *) * number_of_shards);
  p->insert_reaction_stmt = malloc(sizeof(sqlite3_stmt *) * number_of_shards);


  int shard;

  for (shard = 0; shard < number_of_shards; shard++) {
    p->create_reactions_table[shard] = create_reactions_table_sql(shard);
    p->insert_reaction[shard] = insert_reaction_sql(shard);
  }

  p->create_metadata_table = create_metadata_table_sql;
  p->insert_metadata = insert_metadata_sql;

  int rc = sqlite3_prepare_v2(p->db, p->insert_metadata, -1, &p->insert_metadata_stmt, NULL);
  if (rc != SQLITE_OK) {
    printf("new_to_database_sql error: %s", sqlite3_errmsg(p->db));
    return NULL;
  }

  for (shard = 0; shard < number_of_shards; shard++) {
    rc = sqlite3_prepare_v2(p->db,
                            p->insert_reaction[shard],
                            -1,
                            p->insert_reaction_stmt + shard,
                            NULL);

    if (rc != SQLITE_OK) {
      printf("new_to_database_sql error: %s", sqlite3_errmsg(p->db));
      return NULL;
    }
  }

  return p;
}

void free_to_database_sql(ToDatabaseSQL *p) {
  int i;

  for (i = 0; i < p->number_of_shards; i++) {
    free(p->create_reactions_table[i]);
    free(p->insert_reaction[i]);
    sqlite3_finalize(p->insert_reaction_stmt[i]);
  }

  sqlite3_finalize(p->insert_metadata_stmt);
  free(p->create_reactions_table);
  free(p->insert_reaction);
  sqlite3_close(p->db);
  free(p);
}

int create_tables(ToDatabaseSQL *p) {
  char *err;
  int shard;
  sqlite3_exec(p->db, p->create_metadata_table, NULL, NULL, &err);
  if (err) {
    printf("create_tables: %s",err);
    sqlite3_free(err);
    return -1;
  }

  for (shard = 0; shard < p->number_of_shards; shard++) {
    sqlite3_exec(p->db, p->create_reactions_table[shard], NULL, NULL, &err);
    if (err) {
      printf("create_tables: %s", err);
      sqlite3_free(err);
      return -1;
    }
  }

  return 0;
}

void insert_metadata(ToDatabaseSQL *p,
                    int number_of_species,
                    int number_of_reactions,
                    int shard_size) {

  sqlite3_bind_int(p->insert_metadata_stmt, 1, number_of_species);
  sqlite3_bind_int(p->insert_metadata_stmt, 2, number_of_reactions);
  sqlite3_bind_int(p->insert_metadata_stmt, 3, shard_size);
  sqlite3_step(p->insert_metadata_stmt);
}

void insert_reaction(ToDatabaseSQL *p, int reaction_id, char *reaction_string,
                     int number_of_reactants, int number_of_products,
                     int reactant_1, int reactant_2, int product_1,
                     int product_2, double rate) {

  int shard = reaction_id / p->shard_size;
  sqlite3_stmt *stmt = p->insert_reaction_stmt[shard];

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    sqlite3_bind_int(stmt, 1, reaction_id);
    sqlite3_bind_text(stmt, 2, reaction_string, -1, NULL);
    sqlite3_bind_int(stmt, 3, number_of_reactants);
    sqlite3_bind_int(stmt, 4, number_of_products);
    sqlite3_bind_int(stmt, 5, reactant_1);
    sqlite3_bind_int(stmt, 6, reactant_2);
    sqlite3_bind_int(stmt, 7, product_1);
    sqlite3_bind_int(stmt, 8, product_2);
    sqlite3_bind_double(stmt, 9, rate);
    sqlite3_step(stmt);

}

FromDatabaseSQL *new_from_database_sql(int number_of_shards) {
  FromDatabaseSQL *p = malloc(sizeof(FromDatabaseSQL));
  p->number_of_shards = number_of_shards;
  p->get_reaction = malloc(sizeof(char *) * number_of_shards);

  int i;

  for (i = 0; i < number_of_shards; i++) {
    p->get_reaction[i] = get_reaction_sql(i);
  }

  return p;
}

void free_from_database_sql(FromDatabaseSQL *p) {

  int i;

  for (i = 0; i < p->number_of_shards; i++) {
    free(p->get_reaction[i]);
  }

  free(p->get_reaction);
  free(p);

}
