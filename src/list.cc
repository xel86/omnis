#include "list.h"

#include <arpa/inet.h>

#include <cstdio>
#include <cstdlib>

void ip_list_push_front(struct ip_list **head, const struct in_addr ip) {
    struct ip_list *new_ip = (struct ip_list *)malloc(sizeof(struct ip_list));

    new_ip->ip_address = ip;
    new_ip->next = *head;
    *head = new_ip;
}

void ip_list_push_back(struct ip_list **head, const struct in_addr ip) {
    if (*head == NULL) {
        *head = (struct ip_list *)malloc(sizeof(struct ip_list));
        (*head)->ip_address = ip;
        (*head)->next = NULL;
        return;
    }

    struct ip_list *cursor = *head;
    while (cursor->next != NULL) {
        cursor = cursor->next;
    }

    cursor->next = (struct ip_list *)malloc(sizeof(struct ip_list));

    cursor->next->ip_address = ip;
    cursor->next->next = NULL;
}

int ip_list_contains(struct ip_list &ip_list, const in_addr_t ip) {
    if (ip_list.ip_address.s_addr == ip) return 1;

    struct ip_list *cursor = &ip_list;
    while (cursor != NULL) {
        if (cursor->ip_address.s_addr == ip) {
            return 1;
        }
        cursor = cursor->next;
    }

    return 0;
}

void print_ip_list(struct ip_list *ip_list) {
    struct ip_list *cursor = ip_list;
    if (ip_list == NULL) {
        printf("[]");
        return;
    }

    printf("ip_list: [\n");
    while (cursor != NULL) {
        printf("          %s\n", inet_ntoa(cursor->ip_address));
        cursor = cursor->next;
    }
    printf("         ]\n");
}
