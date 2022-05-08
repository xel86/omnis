#include "cli.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
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

    if (show >= 0 && sorted.size() > show) {
        sorted.resize(show);
    }

    printf("\n| Application      | Rx        | Tx        |\n");
    for (const auto &app : sorted) {
        char rx[15], tx[15];
        printf("--------------------------------------------\n");
        printf("| %-16s | %-9s | %-9s |\n", app.name,
               bytes_to_human(rx, app.pkt_rx), bytes_to_human(tx, app.pkt_tx));
    }
    printf("\n");
}

void display_app_usage_table(std::string &name, struct timeframe start,
                             struct timeframe end, struct timeframe gap) {
    std::vector<struct application> gaps;
    std::vector<time_t> labels;

    db_fetch_app_usage_between_timeframes(gaps, labels, name, start, end, gap);

    if (gaps.empty()) {
        printf("Could not find application with name %s in database\n",
               name.c_str());
        return;
    }

    char from[30], to[30];
    printf("\nHistorical Account of %s from %s to %s:\n", name.c_str(),
           timestamp_to_human(from, labels[0]),
           timestamp_to_human(to, labels[gaps.size() - 1]));

    printf("\n| Timeframe | Rx        | Tx        |\n");
    int i = 0;
    for (const auto &app : gaps) {
        char rx[15], tx[15], label[30];
        printf("-------------------------------------\n");
        printf("| %-9s | %-9s | %-9s |\n", timestamp_to_human(label, labels[i]),
               bytes_to_human(rx, app.pkt_rx), bytes_to_human(tx, app.pkt_tx));
        i++;
    }
    printf("\n");
}
