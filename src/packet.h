#ifndef PACKET_H
#define PACKET_H

#include <netinet/in.h>

#include <ctime>

/* packet direction */
enum direction {
    UNKNOWN_DIRECTION,
    INCOMING_DIRECTION,
    OUTGOING_DIRECTION,
};

struct packet {
    uint8_t protocol;           /* transport protocol used (tcp, udp, ...) */
    struct in_addr source_ip;   /* source ip address */
    struct in_addr dest_ip;     /* destination ip address */
    unsigned short source_port; /* source port */
    unsigned short dest_port;   /* destination port */
    int len;                    /* Total length of packet */
    int header_len;             /* Length of all headers present combined */
    enum direction direction;   /* Is packet sent or received? */
    time_t time;                /* Unix timestamp when packet was captured */
};

void print_packet(struct packet *packet, struct application *app);

/*
 * Uses the previously found values from get_local_ip_addresses to determine if
 * the packet is being received or transmitted. Sets the packet direction value
 * in the provided struct, and returns the direction enum value.
 */
enum direction find_packet_direction(struct packet *packet);

#endif
