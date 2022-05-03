#include "omnis.h"

#include <pcap.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>

#include <chrono>
#include <mutex>
#include <thread>

#include "args.h"
#include "cli.h"
#include "database.h"
#include "human.h"
#include "list.h"
#include "packet.h"
#include "proc.h"
#include "sniffer.h"

FILE *g_log;

struct args g_args;

void daemonize() {
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Failed to fork into daemon process. Exiting.");
        exit(EXIT_FAILURE);
    }

    /* Terminate parent process. */
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) {
        fprintf(stderr,
                "Failed to set child process as session leader. Exiting.");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Failed to fork into daemon process. Exiting.");
        exit(EXIT_FAILURE);
    }

    /* Terminate parent process. */
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);

    /* Change the working directory to the root directory */
    chdir("/");

    /* Close open file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* syslog start message */
    openlog("omnis", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Omnis daemon started.");
    closelog();

    g_log = fopen("/var/log/omnis.log", "w");

    pid_t new_pid = getpid();
    fprintf(g_log, "Omnis daemon started with PID %d\n", new_pid);
    fflush(g_log);

    /* Don't block context switch */
    sleep(1);
}

int main(int argc, char **argv) {
    parse_args(argc, argv, &g_args);

    if (!g_args.daemon) {
        g_log = stdout;
        db_load();

        display_usage_table(g_args.time, g_args.sort, g_args.rows_shown);
        exit(1);
    }

    daemonize();
    db_load();

    pcap_if_t *devices, *device;
    pcap_t *handle;
    int packet_count_limit = 1;
    int timeout_limit = 100;  // milliseconds
    char error_buffer[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&devices, error_buffer)) {
        fprintf(g_log, "error finding device: %s\n", error_buffer);
        return 1;
    }

    for (device = devices; device != NULL; device = device->next) {
        if (g_args.debug)
            fprintf(g_log, "device found: %s | %s\n", device->name,
                    device->description);
    }

    device = devices;
    fprintf(g_log, "Opening device %s for sniffing\n", device->name);
    handle = pcap_open_live(device->name, BUFSIZ, packet_count_limit,
                            timeout_limit, error_buffer);

    if (handle == NULL) {
        fprintf(g_log, "Could not open device %s with error %s", device->name,
                error_buffer);
        return 2;
    }

    get_local_ip_addresses(device->name);
    refresh_proc_mappings();

    std::thread database_update_loop(db_update_loop);
    pcap_loop(handle, -1, packet_handler, NULL);

    return 0;
}
