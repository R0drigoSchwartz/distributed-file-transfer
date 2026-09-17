#ifndef DEFS_H
#define DEFS_H

#include <stddef.h>
#define PORT 6000
#define OUTPUT_DIR "resultados"
#define FILENAME_SIZE 256
#define PATH_SIZE (sizeof(OUTPUT_DIR) + FILENAME_SIZE + 1)
#define HEADER_SIZE FILENAME_SIZE
#define BUFFER_SIZE (32 * 1024 - HEADER_SIZE)
#define DATAGRAM_SIZE 32 * 1024
#define THREAD_COUNT 10

typedef struct {
    char filename[FILENAME_SIZE];
} DatagramHeader;

typedef struct Datagram {
    DatagramHeader header;
    char data[BUFFER_SIZE];
} Datagram;

Datagram* init_datagram(const char *);

#endif
