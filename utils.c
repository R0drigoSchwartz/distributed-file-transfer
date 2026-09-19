#ifndef UTILS_C
#define UTILS_C

#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <string.h>
#include "utils.h"


long get_file_size(const char *file_name) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    
    fclose(file);
    
    return file_size;
}

unsigned char* hash_file(const char *file_name) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        return NULL;
    }

    unsigned char *hash = malloc(HASH_SIZE);
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    int ok = hash != NULL && ctx != NULL && EVP_DigestInit_ex(ctx, EVP_md5(), NULL) == 1;

    unsigned char buffer[4096];
    size_t bytes_read;
    while (ok && (bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ok = EVP_DigestUpdate(ctx, buffer, bytes_read) == 1;
    }

    ok = ok && !ferror(file) && EVP_DigestFinal_ex(ctx, hash, NULL) == 1;

    EVP_MD_CTX_free(ctx);
    fclose(file);

    if (!ok) {
        free(hash);
        return NULL;
    }

    return hash;
}

void configure_sockaddr(struct sockaddr_in *server_addr, in_addr_t ip, uint16_t port) {
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = ip;
}

Datagram* init_datagram(const char *file_name, MessageType message_type, long file_size, const unsigned char* file_hash) {
    Datagram *datagram = malloc(sizeof(Datagram));
    if (datagram == NULL) {
        return NULL;
    }

    datagram->header.message_type = message_type;
    datagram->header.file_size = file_size;
    strncpy(datagram->header.file_name, file_name, FILE_NAME_SIZE - 1);
    memcpy(datagram->header.file_hash, file_hash, HASH_SIZE);
    datagram->header.file_name[FILE_NAME_SIZE - 1] = '\0';

    return datagram;
}


#endif
