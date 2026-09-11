#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

#include "defs.h"
#include "utils.h"


int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

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

    FILE *file_ptr = fopen("file_transfered.txt", "wb");
    if (file_ptr == NULL) {
        perror("File creation failed");
        exit(EXIT_FAILURE);
    }

    Datagram datagram;
    char ack = 1;
    client_addr_len = sizeof(client_addr);
    int k = 0;
    while (true) {
        int nbytes = recvfrom(server_fd, &datagram, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (nbytes < 0) {
            continue;
        }
        printf("Received: %d - %d", nbytes, k++);
        fflush(stdout);
        fwrite(datagram.data, 1, nbytes, file_ptr);
        if (fflush(file_ptr)) {
            perror("fflush did no work on server");
            break;
        }
        sendto(server_fd, &ack, strlen(&ack), MSG_CONFIRM, (const struct sockaddr*)&client_addr, client_addr_len);
    }

    close(server_fd);
    fclose(file_ptr);
    return 0;
}
