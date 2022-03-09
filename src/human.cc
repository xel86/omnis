#include "human.h"

#include <cmath>
#include <cstdio>

const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};

char *bytes_to_human_readable(char *str, double bytes, unsigned seconds) {
    bytes = bytes / seconds;

    int i = 0;
    while (bytes > 1000) {
        bytes /= 1000;
        i++;
    }

    sprintf(str, "%.*f %s/s", i, bytes, units[i]);
    return str;
}
