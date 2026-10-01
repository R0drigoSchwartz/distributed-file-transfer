#include <errno.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <pthread.h>

#include "defs.h"
#include "utils.h"


void* receive_datagram(void *args);
void receive_file(int server_fd, Datagram* datagram, int nbytes, struct sockaddr_in *client_addr, socklen_t client_addr_len, const char *dir);
void send_file_info(int server_fd, Datagram* datagram, struct sockaddr_in *client_addr, socklen_t client_addr_len, const char *dir);
void delete_file(char* file_name);
bool validate_dir(const char *dir);
int validate_port(const char *port);
bool str_is_numeric(const char *number);
int validate_args(int argc, char *argv[], int *port, const char **dir);
void write_file_status(Datagram *datagram, const char *local_file);
long is_file_name_in_file_status(const char *file_name, FILE *file_ptr);


int main(int argc, char *argv[]) {
    int port = -1;
    const char *dir = NULL;

    int arg_valid = validate_args(argc, argv, &port, &dir);
    if (arg_valid != 1) {
        return EXIT_FAILURE;
    }

    printf("listening in port: %d\n", port);
    printf("Destination directory: %s\n", dir);

    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    configure_sockaddr(&server_addr, INADDR_ANY, port);

    int res = bind(server_fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));
    if (res < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);

    ThreadArgs thread_arg = {server_fd, dir};
    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, receive_datagram, (void*) &thread_arg);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    close(server_fd);
    return 0;
}

void* receive_datagram(void *args) {
    int server_fd = ((ThreadArgs*) args)->server_fd;
    const char *dir = ((ThreadArgs*) args)->dir;
 
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    Datagram datagram;
    client_addr_len = sizeof(client_addr);

    while (true) {
        int nbytes = recvfrom(server_fd, &datagram, sizeof(datagram), 0, (struct sockaddr*)&client_addr, &client_addr_len);

        if (nbytes < 0) {
            continue;
        }

        if ((size_t)nbytes < sizeof(datagram.header)) {
            fprintf(stderr, "Datagram smaller than header\n");
            continue;
        }

        if (datagram.header.message_type == GETINFO) {
            send_file_info(server_fd, &datagram, &client_addr, client_addr_len, dir);
        } else {
            receive_file(server_fd, &datagram, nbytes, &client_addr, client_addr_len, dir);
        }
    }
}

void receive_file(int server_fd, Datagram* datagram, int nbytes, struct sockaddr_in *client_addr, socklen_t client_addr_len, const char *dir) {
    size_t dir_size = strlen(dir);
    char output_path[dir_size + FILE_NAME_SIZE + 1];
    snprintf(output_path, sizeof(output_path), "%s/%s", dir, datagram->header.file_name);

    FILE *file_ptr = fopen(output_path, "ab");
    if (file_ptr == NULL) {
        perror("File creation failed");
        return;
    }

    size_t data_size = (size_t)nbytes - sizeof(datagram->header);
    if (get_file_size(output_path) == datagram->header.current_seek) {
        fwrite(datagram->data, 1, data_size, file_ptr);
    }
    
    if (fflush(file_ptr)) {
        perror("fflush did no work on server");
        fclose(file_ptr);
        return;
    }

    write_file_status(datagram, output_path);

    fclose(file_ptr);

    // TODO: is sufficient an ACK like this? Return the current_seek?
    ServerAck ack = {'1'};
    sendto(server_fd, &ack, sizeof(ack), MSG_CONFIRM, (const struct sockaddr*)client_addr, client_addr_len);
}

void send_file_info(int server_fd, Datagram* datagram, struct sockaddr_in *client_addr, socklen_t client_addr_len, const char *dir) {
    ServerAnswer server_answer = {0};

    size_t dir_size = strlen(dir);
    char file_name[dir_size + FILE_NAME_SIZE + 1];
    snprintf(file_name, sizeof(file_name), "%s/%s", dir, datagram->header.file_name);

    long file_size = get_file_size(file_name);

    if (file_size == -1) {
        // File doesn't exists!
        server_answer.file_status = NOT_EXISTS;
    } else if (file_size < datagram->header.file_size) {
        server_answer.file_status = INCOMPLETE;
        server_answer.file_offset = file_size;
    } else if (file_size > datagram->header.file_size) {
        delete_file(file_name);
        server_answer.file_status = CORRUPTED;
    } else {
        unsigned char *file_hash = hash_file(file_name);

        if (file_hash != NULL && memcmp(file_hash, datagram->header.file_hash, HASH_SIZE) == 0) {
            // File is complete
            server_answer.file_status = COMPLETE;
        } else {
            delete_file(file_name);
            server_answer.file_status = CORRUPTED;
        }

        free(file_hash);
    }

    sendto(server_fd, &server_answer, sizeof(server_answer), MSG_CONFIRM, (const struct sockaddr*)client_addr, client_addr_len);
}

void delete_file(char* file_name) {
    if (remove(file_name) == 0) {
        printf("File deleted successfully.\n");
    } else {
        printf("Error: Unable to delete the file.\n");
    }
}

bool validate_dir(const char *dir) {
    if (mkdir(dir, 0755) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create directory");
            return false;
        }

        struct stat info;
        if (stat(dir, &info) == -1) {
            perror("Could not access the directory");
            return false;
        }

        if (!S_ISDIR(info.st_mode)) {
            fprintf(stderr, "The path is not a directory");
            return false;
        }
    }

    return true;
}

bool str_is_numeric(const char *number) {
    while (*number) {
        if (isdigit(*number++) == 0) return false;
    }
    return true;
}

int validate_port(const char *ptr_port) {
    if (!str_is_numeric(ptr_port)) {
        return -1;
    }

    int port = atoi(ptr_port);

    if (port < 1024 || port > 65535) {
        return -1;
    }
    return port;
}

int validate_args(int argc, char *argv[], int *port, const char **dir) {
    if (argc > 3) {
        fprintf(stderr,
                "Use: %s [port | directory] or %s port directory\n",
                argv[0], argv[0]);
        return -1;
    }

    *port = DEFAULT_PORT;
    *dir = DEFAULT_DIR;
    const char *port_arg = NULL;

    if (argc == 2) {
        if (argv[1][0] != '\0' && str_is_numeric(argv[1])) {
            port_arg = argv[1];
        } else {
            *dir = argv[1];
        }
    } else if (argc == 3) {
        port_arg = argv[1];
        *dir = argv[2];
    }

    if (port_arg != NULL) {
        *port = validate_port(port_arg);
        if (*port < 0) {
            fprintf(stderr, "You should provide a valid PORT: %s\n", port_arg);
            return -1;
        }
    }

    if (!validate_dir(*dir)) {
        return -1;
    }

    return 1;
}

void write_file_status(Datagram *datagram, const char *local_file) {
    FILE *file_ptr = fopen(FILE_STATUS, "r+");
    if (file_ptr == NULL && errno == ENOENT) {
        file_ptr = fopen(FILE_STATUS, "w+");
    }
    if (file_ptr == NULL) {
        perror("Error opening file_status.txt!");
        return;
    }

    long write_offset = is_file_name_in_file_status(datagram->header.file_name, file_ptr);
    char hash_hex[HASH_SIZE * 2 + 1]; 
    hash_to_hex(datagram->header.file_hash, hash_hex);
    const char *file_status = "partial";

    if (get_file_size(local_file) == datagram->header.file_size) {
        file_status = "complete";
    }

    fseek(file_ptr, write_offset, SEEK_SET);
    fprintf(file_ptr, "%s %s %-8s\n", datagram->header.file_name, hash_hex, file_status);

    fclose(file_ptr);
}

long is_file_name_in_file_status(const char *file_name, FILE *file_ptr) {
    char line[FILE_NAME_SIZE + HASH_SIZE * 2 + 16];
    size_t name_len = strlen(file_name);
    long line_start = 0;

    while (fgets(line, sizeof(line), file_ptr) != NULL) {
        if (strncmp(line, file_name, name_len) == 0 &&
            (line[name_len] == ' ' || line[name_len] == '\n' || line[name_len] == '\0')) {
                return line_start;
            }
        
        line_start = ftell(file_ptr);
    }

    return line_start;    
}
