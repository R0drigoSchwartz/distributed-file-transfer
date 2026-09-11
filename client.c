#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

#include "utils.h"
#include "defs.h"


static int send_file(int, const struct sockaddr_in *, FILE *, Datagram *);

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

    // Configure server address
    struct sockaddr_in server_addr = {0};
    configure_sockaddr(&server_addr, inet_addr("127.0.0.1"), PORT);

    int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        fclose(file_ptr);
        exit(EXIT_FAILURE);
    }

    // datagram header initialization
    Datagram *datagram = init_datagram(argv[1]);
    if (datagram == NULL) {
        fclose(file_ptr);
        close(client_fd);
        fprintf(stderr, "Failed to initialize datagram\n");
        exit(EXIT_FAILURE);
    }

    int result = send_file(client_fd, &server_addr, file_ptr, datagram);

    close(client_fd);
    fclose(file_ptr);
    free(datagram);

    printf("Program finished with success\n");

    return result;
}

static int send_file(int client_fd, const struct sockaddr_in *server_addr, FILE *file_ptr, Datagram *datagram) {
    socklen_t server_addr_len = sizeof(*server_addr);
    char ack = 0;

    // Send datagrams
    while (true) {
        size_t bytes_read = fread(datagram->data, 1, sizeof(datagram->data), file_ptr);
        if (ferror(file_ptr)) {
            fprintf(stderr, "Failed to read input file\n");
            return EXIT_FAILURE;
        }
        if (bytes_read == 0) {
            break;
        }

        ssize_t n_send = sendto(client_fd, datagram, sizeof(datagram->header) + bytes_read, MSG_CONFIRM, (const struct sockaddr *)server_addr, server_addr_len);
        if (n_send < 0) {
            perror("sendto failed");
            return EXIT_FAILURE;
        }

        // Store the ACK sender's address to verify it matches the expected server.
        struct sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        // wait for ack
        ack = 0;
        ssize_t n_rec = recvfrom(client_fd, &ack, 1, MSG_WAITALL, (struct sockaddr*)&(peer_addr), &peer_addr_len);

        // Check for receive errors and verify that a valid ACK came from the expected server.
        if (n_rec < 0) {
            perror("recvfrom failed");
            return EXIT_FAILURE;
        }
        if (n_rec != 1 ||
            ack != 1 ||
            peer_addr.sin_family != AF_INET ||
            peer_addr.sin_addr.s_addr != server_addr->sin_addr.s_addr ||
            peer_addr.sin_port != server_addr->sin_port) {
            fprintf(stderr, "Invalid ACK or unexpected sender\n");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
