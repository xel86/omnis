#ifndef PROC_H
#define PROC_H

#include <dirent.h>
#include <sys/types.h>

#include <string>
#include <unordered_map>

#include "application.h"

/* /proc/net/tcp lists information about all open sockets on your system.
 * To see for yourself, simply call "cat /proc/net/tcp" in your terminal!
 * Important output for us includes the local_address & port,
 * rem_address & port, and the inode for the socket.
 */

/* key idea: We need a way to connect a packet to a socket on the system.
 * The way we achieve this is by creating a map from information in
 * /proc/net/tcp & udp and connect a hash key based on source ip, source
 * port, dest ip, and dest port with its associated program that owns the
 * socket's inode.
 */
extern std::unordered_map<std::string, struct application *>
    g_packet_process_map;
// extern std::map<std::string, unsigned long> packet_inode_udp;

/* struct to represent socket file-descriptors under a process */
struct socket_fd {
    pid_t pid;           /* process PID that owns socket */
    char *cmdline;       /* cmdline (name) of the process */
    unsigned long inode; /* inode for the socket */
};

void refresh_proc_net_mapping();
void handle_proc_net_line(const char *buffer);

void refresh_proc_pid_mapping();
int entry_is_pid_dir(dirent *entry);
void handle_pid_dir(const char *pid);
void set_cmdline(char **target, const char *pid);
void set_executable_name(char **target, const char *cmdline, size_t len);

unsigned long string_to_ulong(const char *ptr);
int string_to_int(const char *ptr);
#endif
