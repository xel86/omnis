#ifndef ARGS_H
#define ARGS_H

/*
 * used as a global struct that carries user options to determine program
 * state. Includes options given by command line arguments and config files.
 */
struct args {
    bool debug;   /* debug mode */
    bool verbose; /* verbose mode (even more info than debug) */
    bool daemon;  /* flag to determine if we are launched the daemon or cli */
    int interval; /* interval to update database in seconds */
};

void print_help();

int parse_args(int argc, char **argv, struct args *args);

#endif
