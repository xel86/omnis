#ifndef ARGS_H
#define ARGS_H

#include "database.h"

/* Sort preferences for tables, ASC for Ascending, DESC for Descending */
enum sort {
    RX_ASC,
    TX_ASC,
    RX_DESC,
    TX_DESC,
};

/*
 * used as a global struct that carries user options to determine program
 * state. Includes options given by command line arguments and config files.
 */
struct args {
    bool debug;   /* debug mode */
    bool verbose; /* verbose mode (even more info than debug) */
    bool daemon;  /* flag to determine if we are launched the daemon or cli */
    int interval; /* interval to update database in seconds */
    struct timeframe time; /* timeframe to sum application data usage for */
    enum sort sort;        /* Sort preference for table */
    int rows_shown; /* Amount of rows shown on the tabls, truncating rest. */
};

void print_help();

int parse_args(int argc, char **argv, struct args *args);

#endif
