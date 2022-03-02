#include "sniffer.h"

#include <ifaddrs.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pcap.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "global.h"
#include "packet.h"

void get_local_ip_addresses(const char *device_name) {
    struct ifaddrs *interface_addresses, *ifaddress;
    if (getifaddrs(&interface_addresses) < 0) {
        fprintf(stderr,
                "Unable to access local interface addresses from ifaddrs for "
                "device %s",
                device_name);
        exit(1);
    }

    for (ifaddress = interface_addresses; ifaddress != NULL;
         ifaddress = ifaddress->ifa_next) {
        if (ifaddress->ifa_addr == NULL) continue;
        if (strcmp(ifaddress->ifa_name, device_name) != 0) continue;

        struct in_addr ip_address;
        ip_address.s_addr =
            ((struct sockaddr_in *)ifaddress->ifa_addr)->sin_addr.s_addr;

        printf("Local IP Address found for device %s: %s\n", device_name,
               inet_ntoa(ip_address));

        /* If address starts with 192.168.x.x push to the front of the list */
        if (ip_address.s_addr >= 43200) {
            ip_list_push_front(&g_local_ip_list, ip_address);
        } else {
            ip_list_push_back(&g_local_ip_list, ip_address);
        }
    }
}

void handle_tcp_packet(struct packet *packet, const u_char *buffer,
                       int offset) {
    struct tcphdr *tcp_header = (struct tcphdr *)(buffer + offset);

    packet->protocol = IPPROTO_TCP;
    packet->header_len = offset + tcp_header->doff * 4;
    packet->source_port = ntohs(tcp_header->source);
    packet->dest_port = ntohs(tcp_header->dest);
}

void handle_udp_packet(struct packet *packet, const u_char *buffer,
                       int offset) {
    struct udphdr *udp_header = (struct udphdr *)(buffer + offset);

    packet->protocol = IPPROTO_UDP;
    packet->header_len = offset + sizeof(udp_header);
    packet->source_port = ntohs(udp_header->source);
    packet->dest_port = ntohs(udp_header->dest);
}

int packets_read = 0;
void packet_handler(u_char *args, const struct pcap_pkthdr *header,
                    const u_char *buffer) {
    // skip over ethernet header ( always 14 bytes ) and use ip header
    struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    unsigned short ip_header_len = ip_header->ihl * 4;

    struct packet packet;
    packet.len = header->len;
    packet.time = header->ts.tv_sec;
    packet.source_ip.s_addr = ip_header->saddr;
    packet.dest_ip.s_addr = ip_header->daddr;
    find_packet_direction(&packet);

    int offset = ip_header_len + sizeof(struct ethhdr);
    switch (ip_header->protocol) {
        case IPPROTO_TCP:
            handle_tcp_packet(&packet, buffer, offset);
            break;

        case IPPROTO_UDP:
            handle_udp_packet(&packet, buffer, offset);
            break;

        default:
            if (0)  // debug
                printf("\n!! Got Unknown Packet With Protocal Number %d !!\n",
                       ip_header->protocol);
            break;
    }

    packets_read++;
    printf("\rPackets Caught: %d", packets_read);
    if (0)  // debug
        print_packet(&packet);
}
