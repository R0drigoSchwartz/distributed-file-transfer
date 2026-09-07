#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6000
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char const * const message = "Hello, UDP client here.";
    char buffer[BUFFER_SIZE];

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sendto(client_fd, message, strlen(message), MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Message sent to server.\n");

    close(client_fd);
    return 0;
}
