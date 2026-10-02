/*Authors: Rodrigo Schwartz (R0drigoSchwartz) and Vinicius Henrique Ribeiro (vini-ribeiro)*/

#define _GNU_SOURCE

#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <errno.h>

#include "utils.h"
#include "defs.h"


static int send_file(int, const struct sockaddr_in *, FILE *, Datagram *);
static int send_initial_message(int client_fd, const struct sockaddr_in *server_addr, Datagram *datagram, ServerAnswer *server_answe);
static bool validate_ip_addr(const char *ip_addr);
static int validate_args(int argc, char *argv[], const char **ip_addr, int *port);

int main(int argc, char *argv[]) {
    int port = -1;
    const char *ip_addr = NULL;

    int arg_valid = validate_args(argc, argv, &ip_addr, &port);
    if (arg_valid != 1) {
        return EXIT_FAILURE;
    }

    char *file_path = argv[argc - 1];
    const char *file_name = basename(file_path);
    FILE *file_ptr = fopen(file_path, "rb");
    if (file_ptr == NULL) {
        perror("File open failed");;
        exit(EXIT_FAILURE);
    }

    // Configure server address
    struct sockaddr_in server_addr = {0};
    configure_sockaddr(&server_addr, inet_addr(ip_addr), port);

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

    long file_size = get_file_size(file_path);
    unsigned char *file_hash = hash_file(file_path);
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
            printf("File not exists on the server!\n");
            result = send_file(client_fd, &server_addr, file_ptr, datagram);
            break;
        case INCOMPLETE:
            printf("File isn't complete on the server!\n");
            fseek(file_ptr, server_answer.file_offset, SEEK_SET);
            result = send_file(client_fd, &server_addr, file_ptr, datagram);
            break;
        case CORRUPTED:
            printf("File corrupted on the server. It was deleted! Sending again...\n");
            result = send_file(client_fd, &server_addr, file_ptr, datagram);
            break;
        case INVALID:
            printf("A file with that name already exists on the server. You must upload your file with a different name!\n");
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

static bool validate_ip_addr(const char *ip_addr) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip_addr, &(sa.sin_addr));
    return result > 0;
}

static int validate_args(int argc, char *argv[], const char **ip_addr, int *port) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr,
                "Use: %s [ip_addr | port] file_path or %s ip_addr port file_path \n",
                argv[0], argv[0]);
        return -1;
    }

    *ip_addr = DEFAULT_SERVER_ADDR;
    *port = DEFAULT_PORT;
    const char *ip_addr_arg = NULL;
    const char *port_arg = NULL;

    if (argc == 3) {
        if (argv[1][0] != '\0' && str_is_numeric(argv[1])) {
            port_arg = argv[1];
        } else {
            ip_addr_arg = argv[1];
        }
    } else if (argc == 4) {
        ip_addr_arg = argv[1];
        port_arg = argv[2];
    }

    if (ip_addr_arg != NULL) {
        if (validate_ip_addr(ip_addr_arg)) {
            *ip_addr = ip_addr_arg;
        } else {
            fprintf(stderr, "You should provide a valid server IP addr.\n");
            return -1;
        }
    }

    if (port_arg != NULL) {
        *port = validate_port(port_arg);
        if (*port < 0) {
            fprintf(stderr, "You should provide a valid PORT: %s\n", port_arg);
            return -1;
        }
    }

    const char *file_name = basename(argv[argc - 1]);
    if (strcmp(file_name, "") == 0) {
        fprintf(stderr, "File path is not valid.\n");
        return -1;
    }

    return 1;
}
