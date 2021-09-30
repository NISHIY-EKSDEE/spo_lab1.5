#ifndef SPOLAB15_SERVER_H
#define SPOLAB15_SERVER_H

#include <stdint.h>
#include "../utils/message.h"
#include "../datafile/datafile.h"

typedef struct {
    uint16_t port;
    int32_t server_fd;
    datafile *data;
    pthread_mutex_t mutex;
    pthread_t manage_thread;
} server_info;

typedef struct {
    server_info *info;
    int32_t client_socket;
    pthread_t thread;
} client_arguments;

server_info *startup(uint16_t port, datafile *data);

void receive_message(char *client_message_part, int32_t accepted_socket, char *request_json, long content_length);

void send_message(int32_t client_socket, char *message);

void close_server(server_info *info);

_Noreturn void manage_connections(server_info *info);

void work_with_client(client_arguments *args);

char *execute_command(query_info *info, datafile *data);

static server_info *create_server_info(uint16_t port);


#endif //SPOLAB15_SERVER_H
