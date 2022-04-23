#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

#include <string>
#include <unordered_map>

#include "application.h"

/* sqlite3 object to interact with database. This will not be touched outside of
 * db_* functions. */
extern sqlite3 *db;

/* Holds all of the existing application names and their corresponding id */
extern std::unordered_map<std::string, int> application_ids;

/* Used for creating new sqlite3 databases with the schema we designed. */
int db_generate_schema();

/* Opens an existing database or creates a new one if it doesn't exist. Loads
 * existing application ids and names into application maps */
int db_load();

/* Offloads application traffic data from g_application_map accumulated during
 * the time interval into the database and resets
 * the application traffic struct values. */
int db_insert_traffic();

/* Inserts the application name into the application database table,
 * and sets the application id in the struct and also returns it. */
int db_insert_application(struct application *app);

/* Function for a thread to be spawned off of, simply calls db_insert_traffic()
 * every X secs. */
void db_update_loop();

#endif
