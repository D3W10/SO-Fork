#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "sockets.h"
#include "vector_utils.h"
#include "errors.h"

#define SIZE 100000
#define MIN 15
#define MAX 50
#define LOWER_LIMIT 0
#define UPPER_LIMIT 100
#define MAX_BUF 64

char* server_endpoint = "localhost";
int server_port = 5000;

void *client_thread(void *arg) {
    char *connection_type = (char *)arg;
    int socketfd;

    if (strcmp(connection_type, "tcp") == 0)
        socketfd = tcp_socket_client_init(server_endpoint, server_port);
    else if (strcmp(connection_type, "unix") == 0)
        socketfd = un_socket_client_init(UNIX_SOCKET_PATH);

    printf("Initializing a vector of %d values\n", SIZE);

    // allocate a vector of initial values
    int *values = malloc(sizeof(int) * SIZE);
    if (values == NULL) {
        fprintf(stderr, "Erro malloc\n");
        exit(1);
    }

    // allocate a subvector where will be store values in a interval of values
    int *subvalues = malloc(sizeof(int) * SIZE);
    if (subvalues == NULL) {
        fprintf(stderr, "Erro malloc\n");
        exit(1);
    }

    int info[] = { SIZE, MIN, MAX };
    int subvalues_size = 0;

    // initiate initial array of values
    vector_init_rand(values, SIZE, LOWER_LIMIT, UPPER_LIMIT);

    handle_error_system(writen(socketfd, info, sizeof(info)), "Writing to server");
    handle_error_system(writen(socketfd, values, SIZE * sizeof(int)), "Writing to server");

    int buf[MAX_BUF];

    while (read(socketfd, buf, sizeof(buf)) <= 0);
    cpy_buffer(subvalues, buf, sizeof(buf) / sizeof(int), &subvalues_size);

    while (read(socketfd, buf, sizeof(buf)) > 0)
        cpy_buffer(subvalues, buf, sizeof(buf) / sizeof(int), &subvalues_size);

    printf("Values between [%d..%d]: %d\n", info[0], info[1], subvalues_size);

    handle_error_system(close(socketfd), "[cli] closing socket to server");

    free(values);
    free(subvalues);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <number_of_connections> [tcp|unix] [<server_endpoint>] [<server_port>]\n", argv[0]);
        exit(1);
    }

    if (argc == 4)
        server_endpoint = argv[3];
    if (argc == 5)
        server_port = atoi(argv[4]);

    int num_connections = atoi(argv[1]);
    pthread_t threads[num_connections];

    for (int i = 0; i < num_connections; ++i) {
        if (pthread_create(&threads[i], NULL, client_thread, (void *)argv[1]) != 0)
            exit(1);
    }

    for (int i = 0; i < num_connections; ++i) {
        if (pthread_join(threads[i], NULL) != 0)
            exit(1);
    }

    return 0;
}