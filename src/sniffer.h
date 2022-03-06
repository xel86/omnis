#ifndef SNIFFER_H
#define SNIFFER_H

#include <pcap.h>

#include "packet.h"

/* Global linked list of all local ip addresses for the target device */
extern struct ip_list *g_local_ip_list;

/* Searchs device interface for all local ip addresses belonging to it. */
void get_local_ip_addresses(const char *device_name);

void handle_tcp_packet(struct packet *packet, const u_char *buffer, int offset);

void handle_udp_packet(struct packet *packet, const u_char *buffer, int offset);

/* Returns 1 if a UDP packet is a DNS related packet, returns 0 otherwise */
int is_dns_traffic(const struct packet *packet);

/*
 * Function handler that is hooked with libpcap to be executed everytime a
 * packet is captured. This is the source of where most of the logic in
 * Omnis branches from.
 *
 * args are additional arguments to the handler,
 * header is the base packet header provided by pcap,
 * buffer is the raw packet string caught by pcap.
 */
void packet_handler(u_char *args, const struct pcap_pkthdr *header,
                    const u_char *buffer);
#endif
