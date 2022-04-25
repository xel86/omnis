#ifndef LIST_H
#define LIST_H

#include <netinet/in.h>

#include <cstdio>

struct ip_list {
    struct ip_list *next;
    struct in_addr ip_address;
};

/* Push given ip address to front off ip list */
void ip_list_push_front(struct ip_list **head, const struct in_addr ip);

/* Appends given ip address to back off ip list */
void ip_list_push_back(struct ip_list **head, const struct in_addr ip);

/* Returns 1 if ip_list contains the ip address given, 0 if not. */
int ip_list_contains(struct ip_list &ip_list, const in_addr_t ip);

void print_ip_list(struct ip_list *ip_list, FILE *fp);

#endif
