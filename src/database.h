#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

extern sqlite3* db;

int db_generate_schema();

int db_load();

int db_insert_traffic();

void db_update_loop();

#endif
