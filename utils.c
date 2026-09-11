#ifndef UTILS_C
#define UTILS_C

#include "utils.h"

int max(int a, int b) {
    return (a > b) ? a : b;
};

void configure_sockaddr(struct sockaddr_in *server_addr, in_addr_t ip, uint16_t port) {
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = ip;
}

#endif
