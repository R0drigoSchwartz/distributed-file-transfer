/*Authors: Rodrigo Schwartz (R0drigoSchwartz) and Vinicius Henrique Ribeiro (vini-ribeiro)*/

#ifndef DEFS_H
#define DEFS_H

#include <stddef.h>
#include <openssl/md5.h>

#include "hashmap.h"

#define DEFAULT_PORT 6000
#define DEFAULT_DIR "results"
#define DEFAULT_SERVER_ADDR "127.0.0.1"
#define STATUS_FILE "status_file.txt"
#define FILE_NAME_SIZE 256
#define DATAGRAM_SIZE (32 * 1024)
#define BUFFER_SIZE (DATAGRAM_SIZE - sizeof(DatagramHeader))
#define HASH_SIZE MD5_DIGEST_LENGTH
#define THREAD_COUNT 10
#define SOCKETERROR (-1)

typedef struct {
    int server_fd;
    const char *dir;
    HashMap *map;
} ThreadArgs;

typedef enum {
    GETINFO,
    UPLOAD
} MessageType;

typedef enum {
    NOT_EXISTS,
    INCOMPLETE,
    COMPLETE,
    CORRUPTED,
    INVALID
} FileStatus;

typedef struct {
    MessageType message_type;
    char file_name[FILE_NAME_SIZE];
    unsigned char file_hash[HASH_SIZE];
    long file_size;
    long current_seek;
} DatagramHeader;

typedef struct {
    DatagramHeader header;
    char data[BUFFER_SIZE];
} Datagram;

typedef struct {
    char acknowledge;
} ServerAck;

typedef struct {
    FileStatus file_status;
    long file_offset;
} ServerAnswer;

#endif
