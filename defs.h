#ifndef DEFS_H
#define DEFS_H

#include <stddef.h>
#include <openssl/md5.h>

#define PORT 6000
#define OUTPUT_DIR "resultados"
#define FILE_NAME_SIZE 256
#define PATH_SIZE (sizeof(OUTPUT_DIR) + FILE_NAME_SIZE + 1)
#define DATAGRAM_SIZE (32 * 1024)
#define BUFFER_SIZE (DATAGRAM_SIZE - sizeof(DatagramHeader))
#define HASH_SIZE MD5_DIGEST_LENGTH
#define THREAD_COUNT 10


typedef enum {
    GETINFO,
    UPLOAD
} MessageType;

typedef enum {
    NOT_EXISTS,
    INCOMPLETE,
    COMPLETE
} FileStatus;

typedef struct {
    MessageType message_type;
    char file_name[FILE_NAME_SIZE];
    unsigned char file_hash[HASH_SIZE];
    long file_size;
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
