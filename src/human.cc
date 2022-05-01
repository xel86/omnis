#include "human.h"

#include <cmath>
#include <cstdio>

const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};

char *bytes_to_human_overtime(char *str, double bytes, unsigned seconds) {
    bytes = bytes / seconds;

    int i = 0;
    while (bytes > 1000) {
        bytes /= 1000;
        i++;
    }

    sprintf(str, "%.*f %s/s", i, bytes, units[i]);
    return str;
}

char *bytes_to_human(char *str, double bytes) {
    int i = 0;
    while (bytes > 1000) {
        bytes /= 1000;
        i++;
    }

    sprintf(str, "%.2f %s", bytes, units[i]);
    return str;
}
