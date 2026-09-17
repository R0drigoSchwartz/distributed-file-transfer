#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>

#include "defs.h"
#include "utils.h"


void * receive_file(void* args);

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    configure_sockaddr(&server_addr, INADDR_ANY, PORT);

    int res = bind(server_fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));
    if (res < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, receive_file, (void*) &server_fd);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    close(server_fd);
    return 0;
}

void * receive_file(void* args) {
    int * server_fd = (int *) args;
 
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    Datagram datagram;
    char ack = 1;
    client_addr_len = sizeof(client_addr);

   while (true) {
        int nbytes = recvfrom(*server_fd, &datagram, sizeof(datagram), 0, (struct sockaddr*)&client_addr, &client_addr_len);

        printf("olaaaaaaaaaaa");
        
        if (nbytes < 0) {
            continue;
        }
        
        if ((size_t)nbytes < sizeof(datagram.header)) {
            fprintf(stderr, "Datagram smaller than header\n");
            continue;
        }

        char output_path[PATH_SIZE];
        snprintf(output_path, sizeof(output_path), "%s/%s", OUTPUT_DIR, datagram.header.filename);

        FILE *file_ptr = fopen(output_path, "ab");
        if (file_ptr == NULL) {
            perror("File creation failed");
            exit(EXIT_FAILURE);
        }
        
        size_t data_size = (size_t)nbytes - sizeof(datagram.header);
        fwrite(datagram.data, 1, data_size, file_ptr);
        
        if (fflush(file_ptr)) {
            perror("fflush did no work on server");
            break;
        }
        fclose(file_ptr);
        
        sendto(*server_fd, &ack, sizeof(ack), MSG_CONFIRM, (const struct sockaddr*)&client_addr, client_addr_len);
    }

    return NULL;
}
