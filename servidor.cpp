#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6000
#define BUFFER_SIZE 1024


int main() {
    int server_fd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    const char * const welcome_reponse = "Hello UDP server";

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

    client_addr_len = sizeof(client_addr);
    int nbytes = recvfrom(server_fd, &buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
    buffer[nbytes] = '\0';
    printf("Received: %s\n", buffer);

    close(server_fd);
    return 0;
}
