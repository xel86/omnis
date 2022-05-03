#include "cli.h"

#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "application.h"
#include "args.h"
#include "database.h"
#include "human.h"

void display_usage_table(struct timeframe time, enum sort sort, int show) {
    std::unordered_map<std::string, struct application> apps;
    db_fetch_usage_over_timeframe(apps, time);

    std::vector<struct application> sorted;
    for (auto &it : apps) {
        sorted.push_back(it.second);
    }

    if (sort == RX_DESC) {
        std::sort(sorted.begin(), sorted.end(),
                  [](struct application &a, struct application &b) {
                      return a.pkt_rx > b.pkt_rx;
                  });
    } else if (sort == TX_DESC) {
        std::sort(sorted.begin(), sorted.end(),
                  [](struct application &a, struct application &b) {
                      return a.pkt_tx > b.pkt_tx;
                  });
    } else if (sort == RX_ASC) {
        std::sort(sorted.begin(), sorted.end(),
                  [](struct application &a, struct application &b) {
                      return a.pkt_rx < b.pkt_rx;
                  });
    } else if (sort == TX_ASC) {
        std::sort(sorted.begin(), sorted.end(),
                  [](struct application &a, struct application &b) {
                      return a.pkt_tx < b.pkt_tx;
                  });
    }

    if (show >= 0) {
        sorted.resize(show);
    }

    printf("\n| Application      | Rx        | Tx        |\n");
    for (const auto &app : sorted) {
        char rx[15], tx[15];
        printf("-------------------------------------------\n");
        printf("| %-16s | %-9s | %-9s |\n", app.name,
               bytes_to_human(rx, app.pkt_rx), bytes_to_human(tx, app.pkt_tx));
    }
    printf("\n");
}
