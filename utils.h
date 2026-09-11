#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>

int max(int, int);
void configure_sockaddr(struct sockaddr_in *, in_addr_t, uint16_t);

#endif
