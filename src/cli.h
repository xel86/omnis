#ifndef CLI_H
#define CLI_H

#include "args.h"
#include "database.h"

/* Displays table of all applications who have used the network in the past
 * selected days and the amount of traffic received and transmitted */
void display_usage_table(struct timeframe time, enum sort sort);

#endif
