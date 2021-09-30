#include <netinet/in.h>
#include <arpa/inet.h>
#include "client.h"
#include "../utils/my_alloc.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("No required arguments provided: <server_port>");
        return -1;
    }
    uint16_t port = strtoul(argv[1], NULL, BASE_10);
    int32_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(HOSTNAME);

    int connection = connect(server_fd, (const struct sockaddr *) &server_address, sizeof(server_address));
    if (connection == -1) {
        puts("There was an error making a connection to the remote socket");
        return -1;
    }
    char *input = NULL;
    size_t length;
    uint8_t errors;
    puts("Enter CYPHER query or type \"exit\" to leave");
    long count = server_fd;
    char *response = my_alloc(BUFSIZ);
    char response_string [RESPONSE_BUFFER_SIZE] = {0};
    while (count > 0) {
        getline(&input, &length, stdin);
        if(strcmp(input, "exit\n") == 0) break;
        cypher_parse_result_t *result = cypher_parse(
                input, NULL, NULL, CYPHER_PARSE_ONLY_STATEMENTS);
        if (result == NULL) {
            perror("cypher_parse");
            return EXIT_FAILURE;
        }
        // puts("");
        // cypher_parse_result_fprint_ast(result, stdout, 100, NULL, 0);
        // puts("");
        errors = cypher_parse_result_nerrors(result);
        if (errors > 0) {
            puts("Unknown command");
            cypher_parse_result_free(result);
            continue;
        }
        query_info *info = get_query_info(result);
        if(info == NULL) continue;
        char *request = build_client_json_request(info);
        puts(request);
        send_message(server_fd, request);
        bzero(response, BUFSIZ);
        count = recv(server_fd, response, BUFSIZ, 0);
        long content_length = strtol(response, NULL, 10);
        char *response_json = receive_message(server_fd, content_length);
        puts(response_json);
        bzero(response_string, RESPONSE_BUFFER_SIZE);
        parse_json_response(response_json, response_string);
        puts(response_string);
        puts("");
        free_query_info(info);
        cypher_parse_result_free(result);
    }
    // printf("Total allocated memory: %llu (bytes)\n", get_all());
    // printf("Max allocated memory: %llu (bytes)\n", get_max());
    // printf("Current allocated memory: %llu (bytes)\n", get_current());
}