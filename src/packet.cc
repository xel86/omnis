#include "packet.h"

#include <arpa/inet.h>

#include <cstdio>
#include <string>

#include "global.h"
#include "list.h"
#include "proc.h"

enum direction find_packet_direction(struct packet *packet) {
    in_addr_t source_ip = packet->source_ip.s_addr;

    if (ip_list_contains(*g_local_ip_list, source_ip)) {
        packet->direction = OUTGOING_DIRECTION;
        return OUTGOING_DIRECTION;
    } else {
        packet->direction = INCOMING_DIRECTION;
        return INCOMING_DIRECTION;
    }
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

    printf("\n######### %s %s #########\n", direction_string, protocol_string);
    printf("sport: %hu dport: %hu\n", packet->source_port, packet->dest_port);
    printf("sip: %s ", inet_ntoa(packet->source_ip));
    printf("dip: %s\n", inet_ntoa(packet->dest_ip));
    printf("length: %d header len: %d\n", packet->len, packet->header_len);
    printf("time: %lld\n", (long long)packet->time);

    if (app == NULL) {
        printf("FROM APPLICATION: Unknown (for now!)\n");
    } else {
        printf("FROM APPLICATION: %s\n", app->name);
    }
}
