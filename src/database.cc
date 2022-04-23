#include "database.h"

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#include "application.h"
#include "human.h"
#include "proc.h"

sqlite3 *db;
time_t start_time;
int update_interval = 5;

std::unordered_map<std::string, int> application_ids;

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
        fprintf(stderr, "Error creating schema table 'SESSION' with error: %s",
                err);
        exit(1);
    }

    printf("Sqlite3 schema generated successfully\n");
    return 0;
}

int db_load() {
    int err = sqlite3_open("test.db", &db);
    start_time = std::time(NULL);
    sqlite3_stmt *stmt;

    if (err) {
        fprintf(stderr, "Error opening database with error: %s",
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

    printf("Loaded existing database successfully.\n");

    // load application ids

    const char *sql2 = "SELECT id, name FROM Application;";

    sqlite3_prepare_v3(db, sql2, strlen(sql2), 0, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name;
        name = std::string(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));

        printf("Picked up Application [%d] %s\n", id, name.c_str());
        application_ids[name] = id;
    }

    sqlite3_finalize(stmt);
    return 0;
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

    printf("\n[###################################]\n");
    for (const auto &[name, app] : g_application_map) {
        char rx[15], tx[15];
        if (app->pkt_rx > 0 || app->pkt_tx > 0) {
            sqlite3_bind_int(stmt, 1, start_time);
            sqlite3_bind_int(stmt, 2, update_interval);
            sqlite3_bind_int(stmt, 3, app->id);
            sqlite3_bind_int(stmt, 4, app->pkt_tx);
            sqlite3_bind_int(stmt, 5, app->pkt_rx);
            sqlite3_bind_int(stmt, 6, app->pkt_tx_c);
            sqlite3_bind_int(stmt, 7, app->pkt_rx_c);
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
            stderr,
            "Error inserting new application %s into database with err: %s\n",
            app->name, err);
        return 0;
    }

    int new_id = sqlite3_last_insert_rowid(db);
    app->id = new_id;
    application_ids[app->name] = new_id;

    printf("Created new application %s with id %d %d\n", app->name, new_id,
           app->id);

    return new_id;
}

void db_update_loop() {
    while (1) {
        std::this_thread::sleep_for(std::chrono::seconds(update_interval));

        db_insert_traffic();
    }
}
