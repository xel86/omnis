#include "proc.h"

#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <unordered_map>

#include "database.h"

extern int errno;

/* ipv6 size + seperator, max 5 digit port number + seperator,
 * ipv6 size + seperator, max 5 digit port number + null char. */
const int HASHKEYSIZE = (INET6_ADDRSTRLEN + 5) + 1 + (INET6_ADDRSTRLEN + 5) + 1;

std::mutex g_applications_lock;

std::unordered_map<std::string, std::shared_ptr<struct application>>
    g_packet_process_map;

std::unordered_map<std::string, std::shared_ptr<struct application>>
    g_application_map;

// temporary maps to use to combine into global g_packet_process_map
std::unordered_map<std::string, unsigned long> temp_inode_map;
std::unordered_map<unsigned long, std::shared_ptr<struct application>>
    temp_process_map;

void refresh_proc_mappings() {
    /* TODO: lazy? Could we avoid having to deallocate all these pointers?
     * Perhaps keep a running set of pid's in /proc and only refresh all if we
     * can't find the new socket in a new pid folder? */
    g_packet_process_map.clear();
    g_application_map.clear();

    refresh_proc_pid_mapping();
    refresh_proc_net_mapping("/proc/net/tcp");
    refresh_proc_net_mapping("/proc/net/udp");
    refresh_proc_net_mapping("/proc/net/raw");
    for (const auto &elem : temp_inode_map) {
        auto found = temp_process_map.find(elem.second);
        if (found != temp_process_map.end())
            g_packet_process_map[elem.first] = found->second;
        else
            fprintf(stderr,
                    "Could not find socket with inode %lu in any corresponding "
                    "/proc/pid/fd, with hash %s\n",
                    elem.second, elem.first.c_str());
    }

    /* Should we clear these? Perhaps reuse some for efficiency */
    temp_inode_map.clear();
    temp_process_map.clear();
}

/* Credit to nethogs for a lot of these ideas.
 * https://github.com/raboof/nethogs */
void handle_proc_net_line(const char *buffer) {
    char packed_source[64], packed_dest[64];
    int source_port, dest_port;
    unsigned long inode;

    /* Unpack the information from a /proc/net/tcp line. */
    int matches =
        sscanf(buffer,
               "%*d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %*X "
               "%*X:%*X %*X:%*X %*X %*d %*d %ld %*512s\n",
               packed_source, &source_port, packed_dest, &dest_port, &inode);

    if (matches != 5) {
        fprintf(stderr, "Malformed line buffer from handle_proc_net_line\n");
        return;
    }

    /* Don't update map if the socket is in TIME_WAIT state. */
    if (inode == 0) return;

    struct in_addr source_ip, dest_ip;
    sscanf(packed_source, "%X", &source_ip.s_addr);
    sscanf(packed_dest, "%X", &dest_ip.s_addr);

    /* Unconnected UDP streams will appear in /proc/net/udp as having a local
     * address of 0.0.0.0:port and a rem address of 0.0.0.0:0 making it
     * impossible to identify these streams while capturing packets with our
     * normal packet hash method. These streams will uniquely have only the UDP
     * port number as the key as opposed to the entire packet hash. Will only be
     * accessed by udp packets. */
    if (source_ip.s_addr == 0 && dest_ip.s_addr == 0 && source_port != 0) {
        char port_hash[10];
        snprintf(port_hash, 10, "UDP-%d", source_port);

        if (0)  // debug
            printf("Adding unconnected UDP Stream with port %d\n", source_port);

        temp_inode_map[port_hash] = inode;
        return;
    }

    /* packet hash is sip:sport-dip:dport */
    char hash[HASHKEYSIZE];
    char source_str[INET6_ADDRSTRLEN], dest_str[INET6_ADDRSTRLEN];

    inet_ntop(AF_INET, &source_ip, source_str, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET, &dest_ip, dest_str, INET6_ADDRSTRLEN);

    snprintf(hash, HASHKEYSIZE, "%s:%d-%s:%d", source_str, source_port,
             dest_str, dest_port);

    if (0)  // debug
        printf("HASHKEYSIZE: %d HASH: %s, for source %s:%d, dest %s:%d\n",
               HASHKEYSIZE, hash, source_str, source_port, dest_str, dest_port);

    temp_inode_map[hash] = inode;
}

void refresh_proc_net_mapping(const char *filename) {
    FILE *proc_net = fopen(filename, "r");
    if (proc_net == NULL) {
        fprintf(stderr, "Could not access %s, error: %s, exiting.", filename,
                strerror(errno));
        exit(1);
    }

    char buffer[8192];

    // First line is the header, unneeded.
    fgets(buffer, sizeof(buffer), proc_net);

    do {
        if (fgets(buffer, sizeof(buffer), proc_net)) {
            handle_proc_net_line(buffer);
        }
    } while (!feof(proc_net));

    fclose(proc_net);
}

void refresh_proc_pid_mapping() {
    DIR *proc = opendir("/proc");

    if (proc == NULL) {
        fprintf(stderr,
                "Could not access the /proc directory, error: %s exiting.",
                strerror(errno));
        std::exit(1);
    }

    dirent *entry;
    while ((entry = readdir(proc))) {
        if (entry_is_pid_dir(entry)) {
            handle_pid_dir(entry->d_name);
        } else {
            continue;
        }
    }
    closedir(proc);
}

int entry_is_pid_dir(dirent *entry) {
    if (entry->d_type != DT_DIR) return 0;

    int len = strlen(entry->d_name);
    for (int i = 0; i < len; i++) {
        if (!isdigit(entry->d_name[i])) return 0;
    }

    return 1;
}

void handle_pid_dir(const char *pid) {
    char fd_dir_name[30];
    size_t dirlen = 10 + strlen(pid);
    snprintf(fd_dir_name, dirlen, "/proc/%s/fd", pid);

    DIR *fd_dir = opendir(fd_dir_name);
    if (fd_dir == NULL) {
        fprintf(
            stderr,
            "Could not access pid file descriptor directory %s, error: %s\n",
            fd_dir_name, strerror(errno));
        return;
    }

    int has_socket = 0, new_app = 0;
    char *cmdline;
    set_cmdline(&cmdline, pid);

    std::shared_ptr<struct application> app;
    auto found = g_application_map.find(cmdline);
    if (found != g_application_map.end()) {
        app = found->second;
        free(cmdline);
        new_app = 0;
    } else {
        new_app = 1;
    }

    dirent *entry;
    while ((entry = readdir(fd_dir))) {
        /* file descriptors are always symbolic links */
        if (entry->d_type != DT_LNK) continue;

        char full_path[41];
        size_t pathlen = dirlen + strlen(entry->d_name) + 1;
        snprintf(full_path, pathlen, "%s/%s", fd_dir_name, entry->d_name);

        char link_name[80];
        int linklen = readlink(full_path, link_name, 79);

        if (linklen < 0) continue;

        link_name[linklen] = '\0';
        if (strncmp(link_name, "socket:[", 8) == 0) {
            unsigned long inode = string_to_ulong(link_name + 8);

            /* If this is the first socket found for the process, initalize
             * application values. If we have already found a socket for this
             * process, point its inode key to the same application. */
            if (!has_socket && new_app) {
                has_socket = 1;

                app = std::make_shared<struct application>();
                app->id = 0;
                app->pkt_rx = 0;
                app->pkt_tx = 0;
                app->pkt_rx_c = 0;
                app->pkt_tx_c = 0;
                app->pkt_tcp = 0;
                app->pkt_udp = 0;
                app->pid = string_to_int(pid);
                app->name = cmdline;
                app->start_time = std::time(NULL);

                db_insert_application(&(*app));

                g_application_map[cmdline] = app;
            }

            temp_process_map[inode] = app;
        }
    }
    closedir(fd_dir);

    /* If no socket file descriptor found for process, free temp cmdline */
    if (!has_socket && new_app) {
        free(cmdline);
        return;
    }
}

/* sets target to the cmdline of the given pid. Allocates memory needed and
 * appends null termination character. */
void set_cmdline(char **target, const char *pid) {
    char path[40];
    size_t len = 15 + strlen(pid);
    snprintf(path, len, "/proc/%s/cmdline", pid);

    FILE *cmdline_file = fopen(path, "r");
    if (cmdline_file == NULL) {
        fprintf(stderr, "Could not open cmdline file for pid %s, error: %s\n",
                pid, strerror(errno));
        *target = (char *)malloc(1);
        (*target)[0] = '\0';
        return;
    }

    char buffer[8192];
    fgets(buffer, sizeof(buffer) - 1, cmdline_file);

    size_t cmdline_len = strlen(buffer);
    set_executable_name(target, buffer, cmdline_len);

    fclose(cmdline_file);
}

const char *interpreted[] = {"python", "python3"};
int is_interpreted(const char *cmp, size_t len) {
    /* checks is the length of the constant interpreted array above. */
    int checks = 2;
    for (int i = 0; i < checks; i++) {
        if (strncmp(cmp, interpreted[i], len) == 0) {
            return 1;
        }
    }

    return 0;
}

void set_executable_name(char **target, const char *cmdline, size_t len) {
    char temp_name[len];

    int new_len = 0;
    while (*cmdline) {
        if (*cmdline == '/') {
            for (int i = 0; i < new_len; i++) temp_name[i] = '\0';

            new_len = 0;
            cmdline++;
            continue;
        }

        if (*cmdline == ' ') {
            break;
        }

        temp_name[new_len] = *cmdline;
        new_len++;
        cmdline++;
    }

    /* Programs that use python, and other interpreted languages, to run will
     * have a slightly ambigious cmdline which we want to avoid. For example
     * streamlink's cmdline /usr/bin/python\0/usr/bin/streamlink has a null
     * character seperator instead of a space. So we skip the potential null
     * character and check to see if the initial executable name is an
     * interpreted language, like python, and if so set the executable name
     * again on what remains. This method will work for both space and null
     * character seperators. */
    cmdline++;
    if (is_interpreted(temp_name, new_len) &&
        (*cmdline != '\0' || *cmdline != ' ')) {
        for (int i = 0; i < new_len; i++) temp_name[i] = '\0';

        set_executable_name(target, cmdline, len - new_len);
        return;
    }

    temp_name[new_len] = '\0';
    *target = (char *)malloc(new_len + 1);
    strcpy(*target, temp_name);
}

unsigned long string_to_ulong(const char *str) {
    unsigned long value = 0;

    while (isdigit(*str)) {
        value *= 10;
        value += *str - '0';
        str++;
    }
    return value;
}

int string_to_int(const char *str) {
    int value = 0;

    while (isdigit(*str)) {
        value *= 10;
        value += *str - '0';
        str++;
    }
    return value;
}

