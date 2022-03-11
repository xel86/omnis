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

#include "list.h"
#include "packet.h"
#include "proc.h"

std::unordered_map<std::string, struct unresolved_buffer> unresolved_packets;
struct ip_list *g_local_ip_list;

inline void try_resolve_packets() {
    if (unresolved_packets.empty()) return;

    refresh_proc_mappings();
    for (const auto &e : unresolved_packets) {
        auto found = g_packet_process_map.find(e.first);
        if (found != g_packet_process_map.end()) {
            found->second->pkt_tx += e.second.pkt_tx;
            found->second->pkt_rx += e.second.pkt_rx;

            fprintf(stderr, "Connected packets to %s\n", found->second->name);
        } else {
            fprintf(
                stderr,
                "Couldn't connect packets (tx: %llu rx: %llu) with hash %s\n",
                e.second.pkt_tx, e.second.pkt_rx, e.first.c_str());
        }
    }

    unresolved_packets.clear();
}

int should_disregard_packet(const struct packet *packet) {
    /* DNS & MDNS Packets will appear as UDP packets, but have no socket
     * associated with it in /proc/net/udp. We must distinguish them somehow,
     * they always come from port 53 or 5353...
     * https://stackoverflow.com/questions/7565300/identifying-dns-packets
     */
    if (packet->dest_port == 53 || packet->source_port == 53) return 1;
    if (packet->dest_port == 5353 && packet->source_port == 5353) return 1;

    /* SSDP packets are in the same situation as above.
     * They are exclusive to UDP packets on port 1900.
     * https://wiki.wireshark.org/SSDP
     */
    if (packet->dest_port == 1900 || packet->source_port == 1900) return 1;

    /* NTP (Network Time Protocol) packets, used for time synchronization. */
    if (packet->dest_port == 123 && packet->source_port == 123) return 1;

    return 0;
}

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

unsigned long long resolve_interval = 0;
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
            if (should_disregard_packet(&packet)) return;

            break;

        default:
            if (0)  // debug
                printf("\n!! Got Unknown Packet With Protocal Number %d !!\n",
                       ip_header->protocol);
            return;
    }

    char hash[HASHKEYSIZE];
    char sip[INET6_ADDRSTRLEN], dip[INET6_ADDRSTRLEN];
    strcpy(sip, inet_ntoa(packet.source_ip));
    strcpy(dip, inet_ntoa(packet.dest_ip));

    if (packet.direction == OUTGOING_DIRECTION)
        snprintf(hash, HASHKEYSIZE, "%s:%d-%s:%d", sip, packet.source_port, dip,
                 packet.dest_port);
    else
        snprintf(hash, HASHKEYSIZE, "%s:%d-%s:%d", dip, packet.dest_port, sip,
                 packet.source_port);

    /* Every 500 packets captured we try to resolve any unresolved packets.
     * This is completely arbitrary, and something else could be better.
     * Could a timed interval potentially be better? */
    if (resolve_interval > 500) {
        try_resolve_packets();
        resolve_interval = 0;
    }

    // TODO: Can we somehow avoid calling find twice for connected UDP sockets?
    auto found = g_packet_process_map.end();
    if (packet.protocol == IPPROTO_UDP) {
        char port_hash[10];
        if (packet.direction == OUTGOING_DIRECTION)
            snprintf(port_hash, 10, "UDP-%d", packet.source_port);
        else
            snprintf(port_hash, 10, "UDP-%d", packet.dest_port);

        found = g_packet_process_map.find(port_hash);
    }

    if (found == g_packet_process_map.end()) {
        found = g_packet_process_map.find(hash);
    }

    if (found == g_packet_process_map.end()) {
        /* Packets that do not already have an associated application will be
         * put into the unresolved_packets map which will act as a buffer for a
         * connection dictated by its packet hash. This is to reduce the amount
         * of times we call refresh_proc_mappings() overall. */
        if (packet.direction == OUTGOING_DIRECTION)
            unresolved_packets[hash].pkt_tx += packet.len;
        else if (packet.direction == INCOMING_DIRECTION)
            unresolved_packets[hash].pkt_rx += packet.len;

        resolve_interval++;
        return;
    }

    /* After this point, the packet successfully resolved to an application */
    struct application *app;
    app = found->second;
    if (packet.direction == OUTGOING_DIRECTION)
        app->pkt_tx += packet.len;
    else if (packet.direction == INCOMING_DIRECTION)
        app->pkt_rx += packet.len;

    resolve_interval++;

    if (0) {  // debug
        if (ip_header->protocol == IPPROTO_TCP ||
            ip_header->protocol == IPPROTO_UDP)
            print_packet(&packet, app);
    }
}
