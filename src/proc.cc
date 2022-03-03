#include "proc.h"

#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

std::unordered_map<std::string, struct application *> g_packet_process_map;

// temporary maps to use to combine into global g_packet_process_map
std::unordered_map<std::string, unsigned long> temp_inode_map;
std::unordered_map<unsigned long, struct application *> temp_process_map;

void refresh_proc_mappings() {
    refresh_proc_pid_mapping();
    refresh_proc_net_mapping("/proc/net/tcp");
    refresh_proc_net_mapping("/proc/net/udp");
    for (const auto &elem : temp_inode_map) {
        g_packet_process_map[elem.first] = temp_process_map[elem.second];
    }
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

    assert(matches == 5);

    /* Don't update map if the socket is in TIME_WAIT state. */
    if (inode == 0) return;

    struct in_addr source_ip, dest_ip;
    sscanf(packed_source, "%X", &source_ip.s_addr);
    sscanf(packed_dest, "%X", &dest_ip.s_addr);

    /* hash is sip:sport-dip:dport without the parentheses */
    int hash_size = (INET_ADDRSTRLEN * 2) + (5 * 2) + 1;
    char hash[hash_size];
    char source_str[INET_ADDRSTRLEN], dest_str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &source_ip, source_str, INET_ADDRSTRLEN - 1);
    inet_ntop(AF_INET, &dest_ip, dest_str, INET_ADDRSTRLEN - 1);

    snprintf(hash, hash_size, "%s:%d-%s:%d", source_str, source_port, dest_str,
             dest_port);

    if (1)  // debug
        printf("HASH: %s, for source %s:%d, dest %s:%d\n", hash, source_str,
               source_port, dest_str, dest_port);

    temp_inode_map[hash] = inode;
}

void refresh_proc_net_mapping(const char *filename) {
    FILE *proc_net = fopen(filename, "r");
    if (proc_net == NULL) {
        fprintf(stderr, "Could not access %s, exiting.", filename);
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

    free(proc_net);
}

void refresh_proc_pid_mapping() {
    DIR *proc = opendir("/proc");

    if (proc == NULL) {
        fprintf(stderr, "Could not access the /proc directory, exiting.");
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

    if (0)  // debug
        for (const auto &elem : temp_process_map) {
            printf("[*] %s\n", elem.second->name);
            printf("    pid: %d inode: %lu\n", elem.second->pid, elem.first);
        }
    free(proc);
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
        fprintf(stderr,
                "Could not access pid file descriptor directory %s, exiting",
                fd_dir_name);
        std::exit(1);
    }

    int has_socket = 0;
    struct application *new_app =
        (struct application *)malloc(sizeof(struct application));

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
            if (!has_socket) {
                has_socket = 1;
                new_app->pid = string_to_int(pid);
                set_cmdline(&new_app->name, pid);
            }

            temp_process_map[inode] = new_app;
        }
    }
    free(fd_dir);

    /* If no socket file descriptor found for process, free temp application */
    if (!has_socket) {
        free(new_app);
        return;
    }
}

void set_cmdline(char **target, const char *pid) {
    char path[40];
    size_t len = 15 + strlen(pid);
    snprintf(path, len, "/proc/%s/cmdline", pid);

    FILE *cmdline_file = fopen(path, "r");
    if (cmdline_file == NULL) {
        fprintf(stderr, "Could not open cmdline file for pid %s", pid);
        *target = (char *)malloc(1);
        (*target)[0] = '\0';
        return;
    }

    char buffer[8192];
    fgets(buffer, sizeof(buffer), cmdline_file);

    size_t cmdline_len = strlen(buffer);
    set_executable_name(target, buffer, cmdline_len);
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

        if (*cmdline == ' ') break;

        temp_name[new_len] = *cmdline;
        new_len++;
        cmdline++;
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
