#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/const.h"
#include "server.h"
#include "../datafile/datafile.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        puts("No required arguments provided: <server_port> <data_file>");
        return -1;
    }
    init_alloc();
    datafile *data = init_data(argv[2]);
    uint16_t port = strtoul(argv[1], NULL, BASE_10);
    server_info *info_ptr = startup(port, data);

    if (info_ptr == NULL) {
        puts("Unable to startup");
        return -1;
    }
    size_t length;
    char *input;
    do {
        getline(&input, &length, stdin);
    } while(strcmp(input, "exit\n") != 0);
    close_server(info_ptr);
    fclose(data->file);
    my_free(info_ptr);
    // printf("Total allocated memory: %llu (bytes)\n", get_all());
    // printf("Max allocated memory: %llu (bytes)\n", get_max());
    // printf("Current allocated memory: %llu (bytes)\n", get_current());
}