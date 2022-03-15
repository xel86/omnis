#ifndef PROC_H
#define PROC_H

#include <dirent.h>
#include <sys/types.h>

#include <mutex>
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

/* Instead of a packet hash being the key, this map has each applications name
 * from a pruned cmdline as a key. */
extern std::unordered_map<std::string, struct application *> g_application_map;

/* Lock for all maps that have a struct application pointer for a value.
 * Since they all share the same set of application pointers it makes sense to
 * just have a single mutex for all of them.
 * Not a reader/writer lock since when we dump the current intervals application
 * data into the database we also reset all of the values. */
extern std::mutex g_applications_lock;

/* Max length of packet hash key for g_packet_process_map */
extern const int HASHKEYSIZE;

/* Refresh both /proc/%d/fd for all pid's and /proc/net/tcp & udp.
 * Creates map that has a key representing the a hash of the source ip & port,
 * and destination ip & port together. The values of the map are pointers to
 * applications. Results update g_packet_process_map*/
void refresh_proc_mappings();

/* Refresh either /proc/net/tcp or /proc/net/udp */
void refresh_proc_net_mapping(const char *filename);
void handle_proc_net_line(const char *buffer);

/* Refresh all file descriptors in each pid folder in /proc */
void refresh_proc_pid_mapping();
int entry_is_pid_dir(dirent *entry);
void handle_pid_dir(const char *pid);

/* Programs made in interpreted languages such as python, will be launched from
 * python's runtime interpreter making the cmdline start with /usr/bin/python.
 * This creates a problem with programs made in python not getting the proper
 * executable name, this function checks if it is one of these cases. */
int is_interpreted(const char *cmp, size_t len);

/* Allocates and sets the value of target to the cmdline of the pid */
void set_cmdline(char **target, const char *pid);

/* Allocates and sets the value of target to a pruned cmdline of the pid.
 * The prunded cmdline only uses the name of the binary executable.
 * "/usr/bin/omnis --debug" -> "omnis" */
void set_executable_name(char **target, const char *cmdline, size_t len);

unsigned long string_to_ulong(const char *ptr);
int string_to_int(const char *ptr);
#endif
