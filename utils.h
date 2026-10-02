/*Authors: Rodrigo Schwartz (R0drigoSchwartz) and Vinicius Henrique Ribeiro (vini-ribeiro)*/

#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>
#include <stdbool.h>
#include "defs.h"


void configure_sockaddr(struct sockaddr_in *, in_addr_t, uint16_t);

Datagram* init_datagram(const char *, MessageType, long, const unsigned char *);

long get_file_size(const char *);

unsigned char* hash_file(const char *);

void hash_to_hex(unsigned char *, char *);

bool str_is_numeric(const char *);

int validate_port(const char *);


#endif
