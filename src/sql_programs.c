#include "sql_programs.h"

char *create_reactions_table(int shard) {
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
