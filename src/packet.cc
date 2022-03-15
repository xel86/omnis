#include "packet.h"

#include <arpa/inet.h>

#include <cstdio>
#include <string>

#include "list.h"
#include "proc.h"
#include "sniffer.h"

enum direction find_packet_direction(struct packet *packet) {
    in_addr_t source_ip = packet->source_ip.s_addr;
    in_addr_t dest_ip = packet->dest_ip.s_addr;

    enum direction direction;
    if (ip_list_contains(*g_local_ip_list, source_ip)) {
        direction = OUTGOING_DIRECTION;
    } else {
        if (ip_list_contains(*g_local_ip_list, dest_ip))
            direction = INCOMING_DIRECTION;
        else
            direction = NOT_OUR_PACKET;
    }

    packet->direction = direction;
    return direction;
}

void print_packet(struct packet *packet, struct application *app) {
    const char *protocol_string;
    const char *direction_string;
    switch (packet->protocol) {
        case IPPROTO_TCP:
            protocol_string = "TCP\0";
            break;
        case IPPROTO_UDP:
            protocol_string = "UDP\0";
            break;
        default:
            protocol_string = "UNKNOWN\0";
            break;
    }

    switch (packet->direction) {
        case OUTGOING_DIRECTION:
            direction_string = "OUTGOING\0";
            break;
        case INCOMING_DIRECTION:
            direction_string = "INCOMING\0";
            break;
        default:
            direction_string = "UNKNOWN\0";
            break;
    }

    fprintf(stderr, "\n######### %s %s #########\n", direction_string,
            protocol_string);
    fprintf(stderr, "sport: %hu dport: %hu\n", packet->source_port,
            packet->dest_port);
    fprintf(stderr, "sip: %s ", inet_ntoa(packet->source_ip));
    fprintf(stderr, "dip: %s\n", inet_ntoa(packet->dest_ip));
    fprintf(stderr, "length: %d header len: %d\n", packet->len,
            packet->header_len);
    fprintf(stderr, "time: %lld\n", (long long)packet->time);

    if (app == NULL) {
        fprintf(stderr, "FROM APPLICATION: Unknown (for now!)\n");
    } else {
        fprintf(stderr, "FROM APPLICATION: %s\n", app->name);
    }
}
