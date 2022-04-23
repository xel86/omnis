#include "database.h"

#include <cstdio>
#include <mutex>
#include <string>
#include <thread>

#include "application.h"
#include "human.h"
#include "proc.h"

sqlite3 *db;

int db_generate_schema() {
    /* Default schema for newly created database */
    std::string schema =
        "CREATE TABLE Session("
        "start          INT                     NOT NULL, "
        "duration_sec   INT                     NOT NULL, "
        "application_id INT                     NOT NULL, "
        "bytes          INT                     NOT NULL, "
        "pkt_tx         INT                     NOT NULL, "
        "pkt_rx         INT                     NOT NULL, "
        "pkt_total      INT                     NOT NULL, "
        "pkt_tcp        INT                     NOT NULL, "
        "pkt_udp        INT                     NOT NULL);"
        "CREATE TABLE Application("
        "id             INTEGER PRIMARY KEY AUTOINCREMENT   NOT NULL, "
        "name           TEXT UNIQUE                         NOT NULL, "
        "colorHex       TEXT                                NOT NULL);";

    char *err;
    int ret = sqlite3_exec(db, schema.c_str(), NULL, NULL, &err);

    if (ret != SQLITE_OK) {
        fprintf(stderr, "Error creating schema table 'SESSION' with error: %s",
                err);
        exit(1);
    }

    printf("Sqlite3 schema generated successfully\n");
    return 0;
}

int db_load() {
    int err = sqlite3_open("test.db", &db);
    sqlite3_stmt *stmt;

    if (err) {
        fprintf(stderr, "Error opening database with error: %s",
                sqlite3_errmsg(db));
        exit(1);
    }

    std::string sql =
        "SELECT EXISTS ( SELECT name FROM sqlite_schema WHERE type='table' AND "
        "name='Session' );";

    sqlite3_prepare_v3(db, sql.c_str(), sql.size(), 0, &stmt, NULL);
    sqlite3_step(stmt);

    if (sqlite3_column_int(stmt, 0) == 0) {
        db_generate_schema();
    }

    sqlite3_finalize(stmt);
    sql.clear();

    printf("Loaded existing database successfully.\n");

    // load application ids

    sqlite3_prepare_v3(db, sql.c_str(), sql.size(), 0, &stmt, NULL);
    return 0;
}

int db_insert_traffic() {
    std::unique_lock<std::mutex> lock(g_applications_lock);

    char *err;
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &err);

    std::string sql =
        "INSERT INTO Session (start, duration_sec, application_id, bytes, "
        "pkt_tx, pkt_rx, pkt_total, pkt_tcp, pkt_udp) VALUES (?, ?, ?, ?, ?, "
        "?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    sqlite3_prepare_v3(db, sql.c_str(), sql.size(), 0, &stmt, NULL);

    time_t current_time = std::time(NULL);

    printf("\n[###################################]\n");
    for (const auto &[name, app] : g_application_map) {
        char rx[15], tx[15];
        if (app->pkt_rx > 0 || app->pkt_tx > 0) {
            sqlite3_bind_int(stmt, 1, app->start_time);
            sqlite3_bind_int(stmt, 2, (current_time - app->start_time));
            sqlite3_bind_int(stmt, 3, app->id);
            sqlite3_bind_int(stmt, 4, (app->pkt_tx + app->pkt_rx));
            sqlite3_bind_int(stmt, 5, app->pkt_tx);
            sqlite3_bind_int(stmt, 6, app->pkt_rx);
            sqlite3_bind_int(stmt, 7, (app->pkt_tx + app->pkt_rx));
            sqlite3_bind_int(stmt, 8, app->pkt_tcp);
            sqlite3_bind_int(stmt, 9, app->pkt_udp);

            int ret = sqlite3_step(stmt);
            if (ret != SQLITE_DONE) {
                fprintf(
                    stderr,
                    "Commit failed while trying to insert session data: %d\n",
                    ret);
            }

            sqlite3_reset(stmt);

            printf("[*] %s\n", name.c_str());
            printf("    rx: %s tx: %s\n",
                   bytes_to_human_readable(rx, app->pkt_rx, 5),
                   bytes_to_human_readable(tx, app->pkt_tx, 5));

            printf("    tcp: %d udp: %d\n", app->pkt_tcp, app->pkt_udp);

            app->pkt_rx = 0;
            app->pkt_tx = 0;
            app->pkt_tcp = 0;
            app->pkt_udp = 0;
        }
    }

    sqlite3_exec(db, "COMMIT TRANSACTION", NULL, NULL, &err);
    sqlite3_finalize(stmt);

    return 0;
}

void db_update_loop() {
    while (1) {
        int interval = 5;
        std::this_thread::sleep_for(std::chrono::seconds(interval));

        db_insert_traffic();
    }
}
