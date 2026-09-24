#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <errno.h>

#include "utils.h"
#include "defs.h"


static int send_file(int, const struct sockaddr_in *, FILE *, Datagram *);
static int send_initial_message(int client_fd, const struct sockaddr_in *server_addr, Datagram *datagram, ServerAnswer *server_answe);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "You should provide the file path.\n");
        exit(EXIT_FAILURE);
    }

    char *file_name = argv[1];
    FILE *file_ptr = fopen(file_name, "rb");
    if (file_ptr == NULL) {
        perror("File open failed");;
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

    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    long file_size = get_file_size(file_name);
    unsigned char *file_hash = hash_file(file_name);
    if (file_size < 0 || file_hash == NULL) {
        fprintf(stderr, "Failed to read file info\n");
        close(client_fd);
        fclose(file_ptr);
        free(file_hash);
        exit(EXIT_FAILURE);
    }

    // datagram header initialization
    Datagram *datagram = init_datagram(file_name, GETINFO, file_size, file_hash);
    if (datagram == NULL) {
        fprintf(stderr, "Failed to initialize datagram\n");
        close(client_fd);
        fclose(file_ptr);
        free(file_hash);
        exit(EXIT_FAILURE);
    }

    ServerAnswer server_answer;
    int exit_status = send_initial_message(client_fd, &server_addr, datagram, &server_answer);
    if (exit_status == EXIT_FAILURE) {
        close(client_fd);
        fclose(file_ptr);
        free(file_hash);
        free(datagram);
        exit(EXIT_FAILURE);
    }

    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int result = EXIT_SUCCESS;
    switch (server_answer.file_status) {
        case NOT_EXISTS:
            printf("File not exists on the server! \n");
            result = send_file(client_fd, &server_addr, file_ptr, datagram);
            break;
        case INCOMPLETE:
            printf("File isn't complete on the server! \n");
            fseek(file_ptr, server_answer.file_offset, SEEK_SET);
            result = send_file(client_fd, &server_addr, file_ptr, datagram);
            break;
        case CORRUPTED:
            printf("File corrupted on the server. It was deleted! \n");
            break;
        case COMPLETE:
            printf("File is complete on the server! \n");
            break;
    }

    close(client_fd);
    fclose(file_ptr);
    free(file_hash);
    free(datagram);

    printf("Program finished with success\n");

    return result;
}

static int send_file(int client_fd, const struct sockaddr_in *server_addr, FILE *file_ptr, Datagram *datagram) {
    socklen_t server_addr_len = sizeof(*server_addr);
    ServerAck server_ack;

    // Send datagrams
    while (true) {
        datagram->header.current_seek = ftell(file_ptr);
        size_t bytes_read = fread(datagram->data, 1, sizeof(datagram->data), file_ptr);
        if (ferror(file_ptr)) {
            fprintf(stderr, "Failed to read input file\n");
            return EXIT_FAILURE;
        }
        if (bytes_read == 0) {
            break;
        }

        datagram->header.message_type = UPLOAD;

        ssize_t n_send = sendto(client_fd, datagram, sizeof(datagram->header) + bytes_read, MSG_CONFIRM, (const struct sockaddr *)server_addr, server_addr_len);
        if (n_send < 0) {
            perror("sendto failed");
            return EXIT_FAILURE;
        }

        // Store the ACK sender's address to verify it matches the expected server.
        struct sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        // wait for ack
        ssize_t bytes_received = recvfrom(client_fd, &server_ack, 1, MSG_WAITALL, (struct sockaddr*)&(peer_addr), &peer_addr_len);

        if (bytes_received == SOCKETERROR && errno == EWOULDBLOCK) {
            // Socket timeout
            fseek(file_ptr, -(long)bytes_read, SEEK_CUR);
            continue;
        }

        // Check for receive errors and verify that a valid ACK came from the expected server.
        if (bytes_received < 0) {
            perror("recvfrom failed");
            return EXIT_FAILURE;
        }

        if (bytes_received != 1 ||
            server_ack.acknowledge != '1' ||
            peer_addr.sin_family != AF_INET ||
            peer_addr.sin_addr.s_addr != server_addr->sin_addr.s_addr ||
            peer_addr.sin_port != server_addr->sin_port) {
            fprintf(stderr, "Invalid ACK or unexpected sender\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int send_initial_message(int client_fd, const struct sockaddr_in *server_addr, Datagram *datagram, ServerAnswer *server_answer) {
    socklen_t server_addr_len = sizeof(*server_addr);

    ssize_t n_send = sendto(client_fd, datagram, sizeof(datagram->header), MSG_CONFIRM, (const struct sockaddr *)server_addr, server_addr_len);
    if (n_send < 0) {
        perror("sendto failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    // wait for the server answer
    ssize_t bytes_received = recvfrom(client_fd, server_answer, sizeof(*server_answer), MSG_WAITALL, (struct sockaddr*)&(peer_addr), &peer_addr_len);

    if (bytes_received == SOCKETERROR && errno == EWOULDBLOCK) {
        // Socket timeout
        printf("Recvfrom timeout waiting for the initial ack.");
        return EXIT_FAILURE;
    }

    // Check for receive errors and verify that a valid server answer came from the expected server.
    if (bytes_received < 0) {
        perror("recvfrom failed");
        return EXIT_FAILURE;
    }
    if (bytes_received != sizeof(*server_answer) ||
        peer_addr.sin_family != AF_INET ||
        peer_addr.sin_addr.s_addr != server_addr->sin_addr.s_addr ||
        peer_addr.sin_port != server_addr->sin_port) {
        fprintf(stderr, "Invalid answer or unexpected sender\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
