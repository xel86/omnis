#include <pcap.h>
#include <unistd.h>

#include <chrono>
#include <mutex>
#include <thread>

#include "args.h"
#include "database.h"
#include "human.h"
#include "list.h"
#include "packet.h"
#include "proc.h"
#include "sniffer.h"

int main(int argc, char **argv) {
    parse_args(argc, argv, &g_args);

    if (!g_args.daemon) {
        printf("Not in daemon mode! :)\n");
        exit(1);
    }

    pcap_if_t *devices, *device;
    pcap_t *handle;
    int packet_count_limit = 1;
    int timeout_limit = 100;  // milliseconds
    char error_buffer[PCAP_ERRBUF_SIZE];

    pid_t pid = getpid();
    printf("PID: %d\n", pid);

    db_load();

    if (pcap_findalldevs(&devices, error_buffer)) {
        printf("error finding device: %s\n", error_buffer);
        return 1;
    }

    for (device = devices; device != NULL; device = device->next) {
        fprintf(stderr, "device found: %s | %s\n", device->name,
                device->description);
    }

    device = devices;
    fprintf(stderr, "Opening device %s for sniffing\n", device->name);
    handle = pcap_open_live(device->name, BUFSIZ, packet_count_limit,
                            timeout_limit, error_buffer);

    if (handle == NULL) {
        fprintf(stderr, "Could not open device %s with error %s", device->name,
                error_buffer);
        return 2;
    }

    get_local_ip_addresses(device->name);
    refresh_proc_mappings();

    std::thread database_update_loop(db_update_loop);
    pcap_loop(handle, -1, packet_handler, NULL);

    return 0;
}
