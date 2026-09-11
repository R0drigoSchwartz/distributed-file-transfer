#ifndef DEFS_H
#define DEFS_H

#define PORT 6000
#define FILENAME_SIZE 256
#define HEADER_SIZE FILENAME_SIZE
#define BUFFER_SIZE (32 * 1024 - HEADER_SIZE)

typedef struct {
    char filename[FILENAME_SIZE];
} DatagramHeader;

typedef struct Datagram {
    DatagramHeader header;
    char data[BUFFER_SIZE];
} Datagram;

Datagram* init_datagram(const char *);

#endif
