#include "database.h"

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "application.h"
#include "human.h"
#include "omnis.h"
#include "proc.h"

sqlite3 *db;

std::unordered_map<std::string, int> application_ids;
time_t time_cursor;

/* 5 second interval to deposit into database */
int update_interval = 5;

time_t timestamp_from_timeframe(const time_t &base,
                                const struct timeframe &time) {
    time_t days = time.days * 24 * 60 * 60;
    time_t hours = time.hours * 60 * 60;
    time_t minutes = time.minutes * 60;

    return base - (days + hours + minutes);
}

time_t timespan_from_timeframe(const struct timeframe &time) {
    time_t days = time.days * 24 * 60 * 60;
    time_t hours = time.hours * 60 * 60;
    time_t minutes = time.minutes * 60;

    return days + hours + minutes;
}

/* I wrote this for non-root before realizing it will be running as root. */
int user_get_or_create_db_path(std::string *path) {
    auto db_path = std::filesystem::path{std::getenv("XDG_CONFIG_HOME")};
    if (db_path.empty()) {
        std::string home = std::getenv("HOME");
        if (home.empty()) {
            fprintf(g_log,
                    "Could not get your HOME directory path. Please make sure "
                    "$HOME is set correctly. Exiting.");
            exit(1);
        }

        auto dot_config = std::filesystem::path{home} / ".config";

        /* If .config doesn't exist, then place the config folder for omnis in a
         * directory called 'omnis' in the home directory; else use
         * .config/omnis */
        if (!std::filesystem::is_directory(dot_config)) {
            db_path = std::filesystem::path{home} / "omnis";
        } else {
            db_path = dot_config / "omnis";
        }
    } else {
        db_path = db_path / "omnis";
    }

    if (!std::filesystem::is_directory(db_path)) {
        int check = mkdir(db_path.c_str(), 0777);

        if (check) {
            fprintf(g_log,
                    "Unable to create omnis directory in %s "
                    "directory for database. Exiting.",
                    db_path.c_str());
            exit(1);
        }
    }

    db_path = db_path / "sqlite.db";

    *path = db_path;
    return 0;
}

int root_get_or_create_db_path(std::string *path) {
    auto db_path = std::filesystem::path{"/var/lib"};

    if (!std::filesystem::is_directory(db_path)) {
        fprintf(g_log,
                "/var/lib not found, needed for omnis database directory. "
                "Exiting.");
        exit(1);
    }

    db_path = db_path / "omnis";

    if (!std::filesystem::is_directory(db_path)) {
        int check = mkdir(db_path.c_str(), 0777);

        if (check) {
            fprintf(g_log,
                    "Unable to create omnis directory in %s "
                    "directory for database. Exiting.",
                    db_path.c_str());
            exit(1);
        }
    }

    db_path = db_path / "omnis.db";

    *path = db_path;
    return 0;
}

int db_generate_schema() {
    /* Default schema for newly created database */
    std::string schema =
        "CREATE TABLE Session("
        "start          INT                     NOT NULL, "
        "durationSec    INT                     NOT NULL, "
        "applicationId  INT                     NOT NULL, "
        "bytesTx        INT                     NOT NULL, "
        "bytesRx        INT                     NOT NULL, "
        "pktTx          INT                     NOT NULL, "
        "pktRx          INT                     NOT NULL, "
        "pktTcp         INT                     NOT NULL, "
        "pktUdp         INT                     NOT NULL);"
        "CREATE TABLE Application("
        "id             INTEGER PRIMARY KEY AUTOINCREMENT   NOT NULL, "
        "name           TEXT UNIQUE                         NOT NULL, "
        "colorHex       TEXT                                DEFAULT '');";

    char *err;
    int ret = sqlite3_exec(db, schema.c_str(), NULL, NULL, &err);

    if (ret != SQLITE_OK) {
        fprintf(g_log, "Error creating schema table 'SESSION' with error: %s",
                err);
        exit(1);
    }

    fprintf(g_log, "Sqlite3 schema generated successfully\n");
    return 0;
}

int db_load() {
    std::string db_path;
    root_get_or_create_db_path(&db_path);

    int err = sqlite3_open(db_path.c_str(), &db);
    sqlite3_stmt *stmt;

    if (err) {
        fprintf(g_log, "Error opening database with error: %s",
                sqlite3_errmsg(db));
        exit(1);
    }

    const char *sql =
        "SELECT EXISTS ( SELECT name FROM sqlite_schema WHERE type='table' AND "
        "name='Session' );";

    sqlite3_prepare_v3(db, sql, strlen(sql), 0, &stmt, NULL);
    sqlite3_step(stmt);

    if (sqlite3_column_int(stmt, 0) == 0) {
        db_generate_schema();
    }

    sqlite3_finalize(stmt);

    db_load_applications(application_ids);

    if (g_args.daemon)
        fprintf(g_log, "Loaded existing database successfully.\n");

    time_cursor = std::time(NULL);
    return 0;
}

void db_load_applications(std::unordered_map<std::string, int> &apps) {
    sqlite3_stmt *stmt;
    const char *sql2 = "SELECT id, name FROM Application;";

    sqlite3_prepare_v3(db, sql2, strlen(sql2), 0, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name;
        name = std::string(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));

        apps[name] = id;
    }

    sqlite3_finalize(stmt);
}

int db_insert_traffic() {
    std::unique_lock<std::mutex> lock(g_applications_lock);

    char *err;
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &err);

    std::string sql =
        "INSERT INTO Session (start, durationSec, applicationId, bytesTx, "
        "bytesRx, pktTx, pktRx, pktTcp, pktUdp) VALUES (?, ?, ?, ?, ?, "
        "?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    sqlite3_prepare_v3(db, sql.c_str(), sql.size(), 0, &stmt, NULL);

    if (g_args.verbose)
        fprintf(g_log, "\n[###################################]\n");

    time_cursor += g_args.interval;
    for (const auto &[name, app] : g_application_map) {
        char rx[15], tx[15];
        if (app->pkt_rx > 0 || app->pkt_tx > 0) {
            sqlite3_bind_int(stmt, 1, time_cursor);
            sqlite3_bind_int(stmt, 2, g_args.interval);
            sqlite3_bind_int(stmt, 3, app->id);
            sqlite3_bind_int(stmt, 4, app->pkt_tx);
            sqlite3_bind_int(stmt, 5, app->pkt_rx);
            sqlite3_bind_int(stmt, 6, app->pkt_tx_c);
            sqlite3_bind_int(stmt, 7, app->pkt_rx_c);
            sqlite3_bind_int(stmt, 8, app->pkt_tcp);
            sqlite3_bind_int(stmt, 9, app->pkt_udp);

            int ret = sqlite3_step(stmt);
            if (ret != SQLITE_DONE) {
                if (g_args.debug)
                    fprintf(g_log,
                            "Commit failed while trying to insert session "
                            "data: %d\n",
                            ret);
            }

            sqlite3_reset(stmt);

            if (g_args.verbose) {
                fprintf(g_log, "[*] %s\n", name.c_str());
                fprintf(g_log, "    rx: %s tx: %s\n",
                        bytes_to_human_overtime(rx, app->pkt_rx, 5),
                        bytes_to_human_overtime(tx, app->pkt_tx, 5));

                fprintf(g_log, "    tcp: %d udp: %d\n", app->pkt_tcp,
                        app->pkt_udp);
            }

            app->pkt_rx = 0;
            app->pkt_tx = 0;
            app->pkt_rx_c = 0;
            app->pkt_tx_c = 0;
            app->pkt_tcp = 0;
            app->pkt_udp = 0;
        }
    }

    sqlite3_exec(db, "COMMIT TRANSACTION", NULL, NULL, &err);
    sqlite3_finalize(stmt);

    return 0;
}

int db_insert_application(struct application *app) {
    auto found = application_ids.find(app->name);
    if (found != application_ids.end()) {
        app->id = found->second;
        return found->second;
    }

    char sql[256], *err;
    snprintf(sql, 256, "INSERT INTO Application (name) VALUES (\"%s\");",
             app->name);

    int ret = sqlite3_exec(db, sql, NULL, NULL, &err);

    if (ret != SQLITE_OK) {
        fprintf(
            g_log,
            "Error inserting new application %s into database with err: %s\n",
            app->name, err);
        return 0;
    }

    int new_id = sqlite3_last_insert_rowid(db);
    app->id = new_id;
    application_ids[app->name] = new_id;

    if (g_args.debug)
        fprintf(g_log, "Inserted new application %s into database\n",
                app->name);

    return new_id;
}

void db_update_loop() {
    while (1) {
        std::this_thread::sleep_for(std::chrono::seconds(g_args.interval));
        db_insert_traffic();

        /* The log file buffer doesn't get flushed for ages if not manually done
         * since we do not output that much information. Force flush it every
         * update interval */
        fflush(g_log);
    }
}

void db_fetch_usage_over_timeframe(
    std::unordered_map<std::string, struct application> &apps,
    struct timeframe time) {
    time_t start_time = timestamp_from_timeframe(std::time(NULL), time);

    /* Flip application_ids map loaded in from db_load() so that id is the key
     * and the name is the value instead */
    std::unordered_map<int, std::string> app_ids;
    for (auto i = application_ids.begin(); i != application_ids.end(); ++i)
        app_ids[i->second] = i->first;

    char sql[256];
    sqlite3_stmt *stmt;

    snprintf(sql, 256, "SELECT * FROM Session WHERE start >= %ld;", start_time);

    sqlite3_prepare_v3(db, sql, strlen(sql), 0, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 2);

        auto found = apps.find(app_ids[id]);
        if (found != apps.end()) {
            struct application &app = found->second;

            app.pkt_tx += sqlite3_column_int64(stmt, 3);
            app.pkt_rx += sqlite3_column_int64(stmt, 4);
            app.pkt_tx_c += sqlite3_column_int(stmt, 5);
            app.pkt_rx_c += sqlite3_column_int(stmt, 6);
            app.pkt_tcp += sqlite3_column_int(stmt, 7);
            app.pkt_udp += sqlite3_column_int(stmt, 8);
        } else {
            struct application new_app;
            strncpy(new_app.name, app_ids[id].c_str(), 16);

            new_app.pkt_tx = sqlite3_column_int64(stmt, 3);
            new_app.pkt_rx = sqlite3_column_int64(stmt, 4);
            new_app.pkt_tx_c = sqlite3_column_int(stmt, 5);
            new_app.pkt_rx_c = sqlite3_column_int(stmt, 6);
            new_app.pkt_tcp = sqlite3_column_int(stmt, 7);
            new_app.pkt_udp = sqlite3_column_int(stmt, 8);

            apps[new_app.name] = new_app;
        }
    }

    sqlite3_finalize(stmt);
}

void db_fetch_app_usage_between_timeframes(
    std::vector<struct application> &time_gaps, std::vector<time_t> &time_edges,
    std::string &name, struct timeframe start, struct timeframe end,
    struct timeframe gap) {
    time_t start_t = timestamp_from_timeframe(std::time(NULL), start);
    time_t end_t = timestamp_from_timeframe(std::time(NULL), end);
    time_t gap_t = timespan_from_timeframe(gap);

    if (start_t > end_t) {
        fprintf(g_log,
                "db_fetch_app_usage_between_timeframes: Can't have a start "
                "date further in time than the end date.");
        std::exit(1);
    }

    /* Calculate number of time gaps (days, months, etc.) that will be populated
     * while iterating between start and end time */
    int n_gaps = (end_t - start_t) / gap_t;
    if ((start_t - end_t) % gap_t) n_gaps++;

    std::unordered_map<int, std::string> app_ids;
    for (auto i = application_ids.begin(); i != application_ids.end(); ++i)
        app_ids[i->second] = i->first;

    auto found = application_ids.find(name);
    if (found == application_ids.end()) {
        return;
    }
    int app_id = found->second;

    /* Create two parallel arrays for each time gap, one that holds the actual
     * data usage accumulation of the application for that time gap, and another
     * for holding the actual unix timestamp edges for each time gap boundry. */
    time_gaps =
        std::vector<struct application>(n_gaps, application(name.c_str()));

    time_edges = std::vector<time_t>(n_gaps);
    time_t last = start_t;
    for (int i = 0; i < n_gaps; i++) {
        time_edges[i] = last + gap_t;
        last = time_edges[i];
    }

    /* If the historical command line argument query, truncate it too 15
     * characters since this is the length of the application names in the
     * database */
    if (name.length() > 15) {
        name.resize(15);
    }

    char sql[256];
    sqlite3_stmt *stmt;

    snprintf(sql, 256,
             "SELECT * FROM Session WHERE (start BETWEEN %ld AND %ld) AND "
             "applicationId=%d;",
             start_t, end_t, app_id);

    sqlite3_prepare_v3(db, sql, strlen(sql), 0, &stmt, NULL);
    time_t time_edge = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        time_t start = sqlite3_column_int64(stmt, 0);
        while (start > time_edges[time_edge]) {
            time_edge++;
        }

        struct application &time_gap = time_gaps[time_edge];

        time_gap.pkt_tx += sqlite3_column_int64(stmt, 3);
        time_gap.pkt_rx += sqlite3_column_int64(stmt, 4);
        time_gap.pkt_tx_c += sqlite3_column_int(stmt, 5);
        time_gap.pkt_rx_c += sqlite3_column_int(stmt, 6);
        time_gap.pkt_tcp += sqlite3_column_int(stmt, 7);
        time_gap.pkt_udp += sqlite3_column_int(stmt, 8);
    }

    sqlite3_finalize(stmt);
}
