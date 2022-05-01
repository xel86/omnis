#include "cli.h"

#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "application.h"
#include "database.h"
#include "human.h"

void display_usage_table(int days) {
    std::unordered_map<std::string, struct application> apps;
    db_fetch_usage_over_timeframe(apps, days);

    std::vector<struct application> sorted;
    for (auto &it : apps) {
        sorted.push_back(it.second);
    }

    std::sort(sorted.begin(), sorted.end(),
              [](struct application &a, struct application &b) {
                  return a.pkt_rx > b.pkt_rx;
              });

    printf("\n| Application      | Rx        | Tx        |\n");
    for (const auto &app : sorted) {
        char rx[15], tx[15];
        printf("-------------------------------------------\n");
        printf("| %-16s | %-9s | %-9s |\n", app.name,
               bytes_to_human(rx, app.pkt_rx), bytes_to_human(tx, app.pkt_tx));
    }
    printf("\n");
}
