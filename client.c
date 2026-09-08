#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

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

    char ack = 0;
    while (true) {
        if ((bytes_read = fread(buffer, 1, BUFFER_SIZE - 1, file_ptr)) <= 0) {
            if (feof(file_ptr)) {
                break;
            }
            continue;
        }
        sendto(client_fd, buffer, bytes_read, MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
        ack = 0;
        socklen_t server_addr_len = sizeof(server_addr);
        int n = recvfrom(client_fd, &ack, 1, MSG_WAITALL, (struct sockaddr*)&server_addr, &server_addr_len);
        if (ack == 0) {
            perror("Did not receive ack equal to 1");
            break;
        }
    }

    close(client_fd);
    fclose(file_ptr);
    return 0;
}
