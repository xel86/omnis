#ifndef HUMAN_H
#define HUMAN_H

/* Converts bytes over a time period to a human readable format.
 * Example 10000 bytes over 10 seconds will be 1.0 KB/s
 * Byte postfix will be determined based on keeping the result below 100.0
 * All values will be in bytes, not bits. i.e MB/s not Mbps.
 * Sets char *str to a null terminated string with the result.
 * char* str MUST be preallocated with enough space for 15 chars */
char *bytes_to_human_overtime(char *str, double bytes, unsigned seconds);

char *bytes_to_human(char *str, double bytes);
#endif
