#ifndef SPOLAB15_MESSAGE_H
#define SPOLAB15_MESSAGE_H
#include "const.h"
#include "linked_list.h"
#include <json-c/json.h>

struct query_info {
    char command_type[COMMAND_TYPE_SIZE];
    linked_list *labels;
    linked_list *props;
    bool has_relation;
    // char rel_name[RELATION_NAME_SIZE];
    linked_list *rel_names;
    linked_list *rel_nodes_labels;
    linked_list *rel_nodes_props;
    linked_list *changed_labels;
    linked_list *changed_props;
};

struct property {
    char key[PROPERTY_KEY_SIZE];
    char value[PROPERTY_VALUE_SIZE];
};

struct match_result {
    linked_list *labels;
    linked_list *props;
};

typedef struct query_info query_info;

typedef struct property property;

typedef struct match_result match_result;

char *build_client_json_request(query_info *info);

query_info *parse_client_json_request(char *json_request);

void parse_json_response(char *json_response, char *response_string);

query_info *init_query_info();

match_result *init_match_result();

void free_query_info(query_info * info);

char *build_json_match_response(linked_list *match_results, uint64_t number);

char *build_json_create_or_delete_response(char *command_type, char *object_type, uint64_t number);

char *build_json_set_or_remove_response(char *command_type, char *object_type, linked_list *changed, uint64_t number);

void free_match_result(linked_list *match_results);

bool by_property_values(void *value, char *to_find_key, char* to_find_value);

static void parse_json_relations(json_object *json_relations, linked_list *rel_names);

static void parse_json_node_labels(json_object *json_node, linked_list *labels); 

static void parse_json_node_props(json_object *json_node, linked_list *props);

static void build_json_relations(json_object *jnode, linked_list *relations_list);

static void build_json_node_labels(json_object *jnode, linked_list *labels_list);

static void build_json_node_props(json_object *jnode, linked_list *props_list);

static void get_node_labels_string(json_object *labels, char *labels_string);

static void get_node_props_string(json_object *props, char *props_string);
#endif
