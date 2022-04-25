#ifndef OMNIS_H
#define OMNIS_H

#include <cstdio>

#include "args.h"

/* Global log file */
extern FILE *g_log;

/* Global args struct containing program options */
extern struct args g_args;

/* Handles forking off the process to turn it into a daemon */
void daemonize();

#endif
