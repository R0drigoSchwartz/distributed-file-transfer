#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6000
#define BUFFER_SIZE 32 * 1024


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "You should provide the file path.\n");
        exit(EXIT_FAILURE);
    }

    FILE *file_ptr = fopen(argv[1], "rb");
    if (file_ptr == NULL) {
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int bytes_read = 0;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE - 1, file_ptr)) > 0) {
        buffer[BUFFER_SIZE - 1] = '\0';
        sendto(client_fd, buffer, strlen(buffer), MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    close(client_fd);
    return 0;
}
