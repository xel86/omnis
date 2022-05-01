#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "application.h"

struct timeframe {
    int days;
    int hours;
    int minutes;
    int seconds;
};

/* sqlite3 object to interact with database. This will not be touched outside of
 * db_* functions. */
extern sqlite3 *db;

/* Holds all of the existing application names and their corresponding id */
extern std::unordered_map<std::string, int> application_ids;

/* Sets full database path in path for an omnis that is running as a non-root
 * user. */
int user_get_or_create_db_path(std::string *path);

/* Sets full database path in path for an omnis that is running as root. */
int root_get_or_create_db_path(std::string *path);

/* Used for creating new sqlite3 databases with the schema we designed. */
int db_generate_schema();

/* Opens an existing database or creates a new one if it doesn't exist. Loads
 * existing application ids and names into application maps */
int db_load();

/* Loads the Application table into a map with the keys being the name of the
 * application and the key being their associated id */
void db_load_applications(std::unordered_map<std::string, int> &apps);

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

/* Iterate over every session for each application in the database for the past
 * specified days and accumulate the traffic used for the time period */
void db_fetch_usage_over_timeframe(
    std::unordered_map<std::string, struct application> &apps,
    struct timeframe time);
#endif
