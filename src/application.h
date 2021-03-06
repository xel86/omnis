#ifndef APPLICATION_H
#define APPLICATION_H

#include <sys/types.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

struct application {
    int id;                    /* database application id */
    pid_t pid;                 /* pid directory for application */
    char name[16];             /* application name (pruned process cmdline) */
    unsigned long long pkt_rx; /* packets received in bytes */
    unsigned long long pkt_tx; /* packets transmitted in bytes */
    int pkt_rx_c;              /* number of packets received */
    int pkt_tx_c;              /* number of packets transmitted */
    int pkt_tcp;               /* number of tcp packets */
    int pkt_udp;               /* number of udp packets */
    time_t start_time;         /* timestamp for when application detected */

    application(const char *comm) {
        id = 0;
        pkt_rx = 0;
        pkt_tx = 0;
        pkt_rx_c = 0;
        pkt_tx_c = 0;
        pkt_tcp = 0;
        pkt_udp = 0;
        pid = 0;
        strncpy(name, comm, 16);
        start_time = std::time(NULL);
    }

    application() {}

    ~application() {}
};

void cleanup_application(struct application *app);

#endif
