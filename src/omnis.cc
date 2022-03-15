#include <pcap.h>
#include <unistd.h>

#include <chrono>
#include <mutex>
#include <thread>

#include "human.h"
#include "list.h"
#include "packet.h"
#include "proc.h"
#include "sniffer.h"

enum debug {
    DEBUG_NONE = 0,
    DEBUG_SOME = 1,
    DEBUG_ALL = 2,
};

/*
 * used as a global struct that carries user options to determine program
 * state. Includes options given by command line arguments and config files.
 */
struct program_state {
    enum debug debug;
};

void dummy_print_status() {
    while (1) {
        int interval = 5;
        std::this_thread::sleep_for(std::chrono::seconds(interval));

        std::unique_lock<std::mutex> lock(g_applications_lock);

        printf("\n[###################################]\n");
        for (const auto &elem : g_application_map) {
            char rx[15], tx[15];
            if (elem.second->pkt_rx > 0 || elem.second->pkt_tx > 0) {
                printf("[*] %s\n", elem.first.c_str());
                printf(
                    "    rx: %s tx: %s\n",
                    bytes_to_human_readable(rx, elem.second->pkt_rx, interval),
                    bytes_to_human_readable(tx, elem.second->pkt_tx, interval));

                printf("    tcp: %d udp: %d\n", elem.second->pkt_tcp,
                       elem.second->pkt_udp);

                elem.second->pkt_rx = 0;
                elem.second->pkt_tx = 0;
                elem.second->pkt_tcp = 0;
                elem.second->pkt_udp = 0;
            }
        }
    }
}

int main(int argc, char **argv) {
    pcap_if_t *devices, *device;
    pcap_t *handle;
    int packet_count_limit = 1;
    int timeout_limit = 100;  // milliseconds
    char error_buffer[PCAP_ERRBUF_SIZE];

    pid_t pid = getpid();
    printf("PID: %d\n", pid);

    if (pcap_findalldevs(&devices, error_buffer)) {
        printf("error finding device: %s\n", error_buffer);
        return 1;
    }

    for (device = devices; device != NULL; device = device->next) {
        printf("device found: %s | %s\n", device->name, device->description);
    }

    device = devices;
    printf("Opening device %s for sniffing\n", device->name);
    handle = pcap_open_live(device->name, BUFSIZ, packet_count_limit,
                            timeout_limit, error_buffer);

    if (handle == NULL) {
        fprintf(stderr, "Could not open device %s with error %s", device->name,
                error_buffer);
        return 2;
    }

    get_local_ip_addresses(device->name);
    refresh_proc_mappings();

    std::thread print_update_loop(dummy_print_status);
    pcap_loop(handle, -1, packet_handler, NULL);

    return 0;
}
