#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

#include "utils.h"
#include "defs.h"


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

    struct sockaddr_in server_addr;
    configure_sockaddr(&server_addr, inet_addr("127.0.0.1"), PORT);
    
    char *filename = argv[1];
    // datagram header initialization
    Datagram *datagram = init_datagram(filename); 

    int client_fd;
    int bytes_read = 0;
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    char ack = 0;
    int k = 0;
    while (true) {
        if ((bytes_read = fread(datagram->data, 1, BUFFER_SIZE, file_ptr)) <= 0) {
            if (feof(file_ptr)) {
                break;
            }
            continue;
        }
        printf("bytes send: %d - %d", bytes_read, k++);
        sendto(client_fd, datagram, bytes_read, MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
        
        // wait for ack
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
    free(datagram);
    
    return 0;
}
