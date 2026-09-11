#ifndef DEFS_C
#define DEFS_C

#include <string.h>
#include <stdlib.h>
#include "defs.h"


Datagram* init_datagram(const char *filename) {
    Datagram *datagram = malloc(sizeof(Datagram));
    if (datagram == NULL) {
        return NULL;
    }

    strncpy(datagram->header.filename, filename, FILENAME_SIZE - 1);
    datagram->header.filename[FILENAME_SIZE - 1] = '\0';

    return datagram;
}


#endif
