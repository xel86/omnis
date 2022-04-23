#ifndef SNIFFER_H
#define SNIFFER_H

#include <pcap.h>

#include "packet.h"

/* Temporary struct for unresolved packets to store their summed information */
struct unresolved_buffer {
    unsigned long long pkt_rx; /* packets received in bytes */
    unsigned long long pkt_tx; /* packets transmitted in bytes */
    int pkt_rx_c;              /* number of packets received */
    int pkt_tx_c;              /* number of packets transmitted */
    int pkt_tcp;               /* number of tcp packets */
    int pkt_udp;               /* number of udp packets */
};

/* Global linked list of all local ip addresses for the target device */
extern struct ip_list *g_local_ip_list;

/* Searchs device interface for all local ip addresses belonging to it. */
void get_local_ip_addresses(const char *device_name);

void handle_tcp_packet(struct packet *packet, const u_char *buffer, int offset);

void handle_udp_packet(struct packet *packet, const u_char *buffer, int offset);

/* Returns 1 if a UDP packet is a packet we should ignore, returns 0 otherwise
 * Packets to be ignored include DNS, MDNS , and SSDP traffic. */
int should_disregard_packet(const struct packet *packet);

/* Attempts to connect any pending packet buffers inside the unresolved_packets
 * map to an application. We only need to call refresh_proc_mappings() once to
 * achieve this for all of the packets in the map. */
void try_resolve_packets();

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
