#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <pthread.h>

#include "defs.h"
#include "utils.h"


void* receive_datagram(void *args);
void receive_file(int server_fd, Datagram* datagram, int nbytes, struct sockaddr_in *client_addr, socklen_t client_addr_len, const char *dir);
void send_file_info(int server_fd, Datagram* datagram, struct sockaddr_in *client_addr, socklen_t client_addr_len, const char *dir);
void delete_file(char* file_name);
bool validate_dir(const char *dir);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "You should provide server PORT and the folder path.\n");
        fprintf(stderr, "Use: %s <port> <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    if (port < 0) {
        fprintf(stderr, "You should provide a valid PORT\n");
        exit(EXIT_FAILURE);
    }

    const char *dir = argv[2];
    if (!validate_dir(dir)) {
        exit(EXIT_FAILURE);
    }
    printf("Destination directory: %s\n", dir);

    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    configure_sockaddr(&server_addr, INADDR_ANY, port);

    int res = bind(server_fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));
    if (res < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);

    ThreadArgs thread_arg = {server_fd, dir};
    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, receive_datagram, (void*) &thread_arg);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    close(server_fd);
    return 0;
}

void* receive_datagram(void *args) {
    int server_fd = ((ThreadArgs*) args)->server_fd;
    const char *dir = ((ThreadArgs*) args)->dir;
 
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    Datagram datagram;
    client_addr_len = sizeof(client_addr);

    while (true) {
        int nbytes = recvfrom(server_fd, &datagram, sizeof(datagram), 0, (struct sockaddr*)&client_addr, &client_addr_len);

        if (nbytes < 0) {
            continue;
        }

        if ((size_t)nbytes < sizeof(datagram.header)) {
            fprintf(stderr, "Datagram smaller than header\n");
            continue;
        }

        if (datagram.header.message_type == GETINFO) {
            send_file_info(server_fd, &datagram, &client_addr, client_addr_len, dir);
        } else {
            receive_file(server_fd, &datagram, nbytes, &client_addr, client_addr_len, dir);
        }
    }
}

void receive_file(int server_fd, Datagram* datagram, int nbytes, struct sockaddr_in *client_addr, socklen_t client_addr_len, const char *dir) {
    size_t dir_size = strlen(dir);
    char output_path[dir_size + FILE_NAME_SIZE + 1];
    snprintf(output_path, sizeof(output_path), "%s/%s", dir, datagram->header.file_name);

    FILE *file_ptr = fopen(output_path, "ab");
    if (file_ptr == NULL) {
        perror("File creation failed");
        return;
    }

    size_t data_size = (size_t)nbytes - sizeof(datagram->header);
    if (get_file_size(output_path) == datagram->header.current_seek) {
        fwrite(datagram->data, 1, data_size, file_ptr);
    }

    if (fflush(file_ptr)) {
        perror("fflush did no work on server");
        fclose(file_ptr);
        return;
    }
    fclose(file_ptr);

    // TODO: is sufficient an ACK like this? Return the current_seek?
    ServerAck ack = {'1'};
    sendto(server_fd, &ack, sizeof(ack), MSG_CONFIRM, (const struct sockaddr*)client_addr, client_addr_len);
}

void send_file_info(int server_fd, Datagram* datagram, struct sockaddr_in *client_addr, socklen_t client_addr_len, const char *dir) {
    ServerAnswer server_answer = {0};

    size_t dir_size = strlen(dir);
    char file_name[dir_size + FILE_NAME_SIZE + 1];
    snprintf(file_name, sizeof(file_name), "%s/%s", dir, datagram->header.file_name);

    long file_size = get_file_size(file_name);

    if (file_size == -1) {
        // File doesn't exists!
        server_answer.file_status = NOT_EXISTS;
    } else if (file_size < datagram->header.file_size) {
        server_answer.file_status = INCOMPLETE;
        server_answer.file_offset = file_size;
    } else if (file_size > datagram->header.file_size) {
        delete_file(file_name);
        server_answer.file_status = CORRUPTED;
    } else {
        unsigned char *file_hash = hash_file(file_name);

        if (file_hash != NULL && memcmp(file_hash, datagram->header.file_hash, HASH_SIZE) == 0) {
            // File is complete
            server_answer.file_status = COMPLETE;
        } else {
            delete_file(file_name);
            server_answer.file_status = CORRUPTED;
        }

        free(file_hash);
    }

    sendto(server_fd, &server_answer, sizeof(server_answer), MSG_CONFIRM, (const struct sockaddr*)client_addr, client_addr_len);
}

void delete_file(char* file_name) {
    if (remove(file_name) == 0) {
        printf("File deleted successfully.\n");
    } else {
        printf("Error: Unable to delete the file.\n");
    }
}

bool validate_dir(const char *dir) {
    if (mkdir(dir, 0755) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create directory");
            return false;
        }

        struct stat info;
        if (stat(dir, &info) == -1) {
            perror("Could not access the directory");
            return false;
        }

        if (!S_ISDIR(info.st_mode)) {
            fprintf(stderr, "The path is not a directory");
            return false;
        }
    }

    return true;
}
