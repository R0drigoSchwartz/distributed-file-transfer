#ifndef DEFS_C
#define DEFS_C

#include <string.h>
#include <stdlib.h>
#include "defs.h"
#include "utils.h"


Datagram* init_datagram(const char *filename) {
    Datagram *datagram = malloc(sizeof(Datagram));

    strncpy(datagram->header.filename, filename, FILENAME_SIZE);
    datagram->header.filename[max(strlen(filename), FILENAME_SIZE) - 1] = '\0';

    return datagram;
}


#endif