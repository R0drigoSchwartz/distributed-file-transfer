#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

#define PORT 6000
#define BUFFER_SIZE 32 * 1024


int main() {
    int server_fd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char const * const welcome_reponse = "Hello UDP server";

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

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

    char ack = 1;
    client_addr_len = sizeof(client_addr);
    while (true) {
        int nbytes = recvfrom(server_fd, &buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (nbytes < 0) {
            continue;
        }
        // printf("Received: %s\n", buffer);
        fwrite(buffer, 1, nbytes, file_ptr);
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
