#include <string.h>
#include "message.h"
#include "my_alloc.h"
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <stdio.h>

char *build_client_json_request(query_info *info) {
    json_object *request = json_object_new_object();

    json_object *command = json_object_new_string((char *) info->command_type);
    json_object_object_add(request, "command", command);
    json_object *nodesArray = json_object_new_array();
    
    json_object *firstNode = json_object_new_object();
    build_json_node_labels(firstNode, info->labels);
    build_json_node_props(firstNode, info->props);
    json_object_array_add(nodesArray, firstNode);

    if (info->has_relation) {
        build_json_relations(request, info->rel_names);
        
        node *current_labels = info->rel_nodes_labels->first;
        node *current_props = info->rel_nodes_props->first;

        for (int i = 0; i < info->rel_nodes_labels->size; ++i) {
            json_object *newNode = json_object_new_object(); 
            build_json_node_labels(newNode, current_labels->value);
            build_json_node_props(newNode, current_props->value);
            json_object_array_add(nodesArray, newNode);

            current_labels = current_labels->next;
            current_props = current_props->next;
        }
        
    }
    if (strcmp(info->command_type, "set") == 0 || strcmp(info->command_type, "remove") == 0) {
        json_object *changedNode = json_object_new_object(); 
        build_json_node_labels(changedNode, info->changed_labels);
        build_json_node_props(changedNode, info->changed_props);
        
        json_object_object_add(request, "changedNode", changedNode);
    }
    json_object_object_add(request, "nodes", nodesArray);

    char *message = strdup(json_object_to_json_string(request));
    json_object_put(request);
    return message;
}

void parse_json_response(char *json_response, char *response_string) {
    json_object *response = json_tokener_parse(json_response);
    json_object *command_obj;
    json_object *number_obj;
    json_object_object_get_ex(response, "command", &command_obj); 
    json_object_object_get_ex(response, "number", &number_obj); 
    char *command_type = json_object_get_string(command_obj);
    char *number = json_object_get_string(number_obj);
    if (command_type == NULL) {
        strcpy(response_string, "BAD REQUEST");
        return;
    }
    if (strcmp(command_type, "match") == 0) {
        sprintf(response_string, "Number of matching nodes: %s", number);
        json_object *nodes; 
        json_object_object_get_ex(response, "nodes", &nodes); 
        size_t nodes_length = json_object_array_length(nodes);
        size_t i; 
        uint32_t offset = strlen(response_string);
        
        for (i=0; i<nodes_length; ++i) {
            json_object *cur_node = json_object_array_get_idx(nodes, i); 

            json_object *labels;
            json_object *props;
            json_object_object_get_ex(cur_node, "props", &props);
            json_object_object_get_ex(cur_node, "labels", &labels);
            
            char labels_string[75] = {0};
            get_node_labels_string(labels, labels_string);
            char props_string[100] = {0};
            get_node_props_string(props, props_string);
            char node_string[200] = {0};
            bzero(node_string, 200);
            sprintf(node_string, "\n%d: labels (%s), props {%s}", i, labels_string, props_string);
            memcpy(response_string + offset, node_string, strlen(node_string));
            offset += strlen(node_string);
        }   
    } else if (strcmp(command_type, "create") == 0 || strcmp(command_type, "delete") == 0) {
        char object_type_string[10];
    
        json_object *type_obj;
        json_object_object_get_ex(response, "type", &type_obj); 

        if (strcmp(json_object_get_string(type_obj), "node") == 0) {
            strcpy(object_type_string, "nodes");
        } else {
            strcpy(object_type_string, "relations");
        }
        char command_type_string[10];
        if (strcmp(command_type, "create") == 0) {
            strcpy(command_type_string, "created");
        } else {
            strcpy(command_type_string, "deleted");
        }
        sprintf(response_string, "Number of %s %s: %s", object_type_string, command_type_string, number);
        
    } else if (strcmp(command_type, "set") == 0 || strcmp(command_type, "remove") == 0) {
        char command_type_string[10];
        if (strcmp(command_type, "set") == 0) {
            strcpy(command_type_string, "set");
        } else {
            strcpy(command_type_string, "removed");
        }
        
        json_object *jobj;
        if (json_object_object_get_ex(response, "labels", &jobj)) {
            char labels_string[BUFSIZ] = {0};
            get_node_labels_string(jobj, labels_string);
            sprintf(response_string, "Number of nodes with labels (%s) %s: %s", labels_string, command_type_string,
                    number);
        } else if (json_object_object_get_ex(response, "props", &jobj)) {
            json_object *prop = json_object_array_get_idx(jobj, 0);
            json_object *key;
            json_object_object_get_ex(prop, "key", &key);
            if (strcmp(command_type, "set") == 0) {
                char prop_string[BUFSIZ];
                bzero(prop_string, BUFSIZ);
                json_object *value;
                json_object_object_get_ex(prop, "value", &value);
                sprintf(prop_string, "%s=%s", json_object_get_string(key), json_object_get_string(value));
                sprintf(response_string, "Number of nodes with attribute '%s' %s: %s", prop_string, command_type_string,
                        number);
            } else {
                sprintf(response_string, "Number of nodes with attribute '%s' %s: %s", json_object_get_string(key), command_type_string,
                        number);
            }
        } else {
            sprintf(response_string, "Nothing changed");
        }
    }
}

query_info *init_query_info() {
    query_info *info = my_alloc(sizeof(query_info));
    info->labels = init_list();
    info->props = init_list();
    info->rel_names = init_list();
    info->rel_nodes_labels = init_list();
    info->rel_nodes_props = init_list();
    info->changed_labels = init_list();
    info->changed_props = init_list();
    info->has_relation = false;
    return info;
}

match_result *init_match_result() {
    match_result *match = my_alloc(sizeof(match_result));
    match->labels = init_list();
    match->props = init_list();
    return match;
}

void free_query_info(query_info *info) {
    free_list(info->labels, false);
    free_list(info->props, true);
    free_list(info->rel_nodes_labels, false);
    free_list(info->rel_nodes_props, true);
    free_list(info->changed_labels, false);
    free_list(info->changed_props, true);
    my_free(info);
}

query_info *parse_client_json_request(char *json_request) {
    query_info *info = init_query_info();
    json_tokener *tok = json_tokener_new();
    json_tokener_reset(tok);
    // json_object *request = json_tokener_parse(json_request);
    json_object *request = json_tokener_parse_ex(tok, json_request, strlen(json_request));
    json_object *command;
    if (!json_object_object_get_ex(request, "command", &command))
        return NULL;
    strcpy(info->command_type, json_object_get_string(command));

    json_object *json_nodes;
    json_object_object_get_ex(request, "nodes", &json_nodes);
    json_object *first_node = json_object_array_get_idx(json_nodes, 0);
    parse_json_node_labels(first_node, info->labels);
    parse_json_node_props(first_node, info->props);
    
    json_object *relations;
    if (json_object_object_get_ex(request, "relations", &relations)) {
        parse_json_relations(relations, info->rel_names);
        info->has_relation = true;
        
        size_t nodes_size; 
        size_t i;
        nodes_size = json_object_array_length(json_nodes);
        for (i=1;i<nodes_size; i++) {
            json_object *new_node = json_object_array_get_idx(json_nodes, i);
            linked_list *new_labels = init_list();
            linked_list *new_props = init_list();
            add_last(info->rel_nodes_labels, new_labels);
            add_last(info->rel_nodes_props, new_props);

            parse_json_node_labels(new_node, new_labels);
            parse_json_node_props(new_node, new_props);
        }
    }
    
    json_object *changed_node;
    if (json_object_object_get_ex(request, "changedNode", &changed_node)) {
        parse_json_node_labels(changed_node, info->changed_labels);
        parse_json_node_props(changed_node, info->changed_props);
    }

    json_tokener_reset(tok);
    json_tokener_free(tok);
    json_object_put(request);

    return info;
}

char *build_json_match_response(linked_list *match_results, uint64_t number) {
    char buffer[BUFSIZ];
    sprintf(buffer, "%lu", number);

    json_object *response = json_object_new_object();
    json_object *command = json_object_new_string((char *) "match");
    json_object_object_add(response, "command", command);
    json_object *json_number = json_object_new_string((char *) buffer);
    json_object_object_add(response, "number", json_number);

    json_object *nodesArray = json_object_new_array();
    for (node *matching_node = match_results->first; matching_node; matching_node = matching_node->next) {
        json_object *node = json_object_new_object();
        build_json_node_labels(node, ((match_result *) matching_node->value)->labels);
        build_json_node_props(node, ((match_result *) matching_node->value)->props);
        json_object_array_add(nodesArray, node);
    }
    json_object_object_add(response, "nodes", nodesArray);

    char *message = strdup(json_object_to_json_string(response));
    json_object_put(response);
    // free_match_result(match_results);
    return message;
}

char *build_json_create_or_delete_response(char *command_type, char *object_type, uint64_t number) {
    char buffer[BUFSIZ];
    sprintf(buffer, "%lu", number);

    json_object *response = json_object_new_object();
    json_object *command = json_object_new_string(command_type);
    json_object_object_add(response, "command", command);
    json_object *type = json_object_new_string(object_type);
    json_object_object_add(response, "type", type);
    json_object *json_number = json_object_new_string((char *) buffer);
    json_object_object_add(response, "number", json_number);

    char *message = strdup(json_object_to_json_string(response));
    json_object_put(response);
    return message;
}

char *build_json_set_or_remove_response(char *command_type, char *object_type, linked_list *changed, uint64_t number) {
    char buffer[BUFSIZ];
    sprintf(buffer, "%lu", number);
    
    json_object *response = json_object_new_object();
    json_object *command = json_object_new_string(command_type);
    json_object_object_add(response, "command", command);
    if (strcmp(object_type, "labels") == 0) {
        build_json_node_labels(response, changed);
    } else {
        build_json_node_props(response, changed);
    }

    json_object *json_number = json_object_new_string((char *) buffer);
    json_object_object_add(response, "number", json_number);

    char *message = strdup(json_object_to_json_string(response));
    json_object_put(response);
    return message;
}

bool by_property_values(void *value, char *to_find_key, char* to_find_value) {
    return strcmp(((property *)value)->key, to_find_key) == 0 && strcmp(((property *)value)->value, to_find_value) == 0;
}

bool by_key(void *value, char *key, char *second_value) {
    return strcmp(((property *)value)->key, key) == 0;
}

void free_match_result(linked_list *match_results) {
    for (node *current = match_results->first; current; current=current->next) {
        free_list(((match_result *)current->value)->labels, false);
        free_list(((match_result *)current->value)->props, true);
        my_free(current->value);
        my_free(current);
    }
}

static void parse_json_relations(json_object *json_relations, linked_list *rel_names) {
    size_t relations_size; 
    size_t i; 

    relations_size = json_object_array_length(json_relations);
    
    for (i=0;i<relations_size; i++) {
        json_object *cur_json_relation = json_object_array_get_idx(json_relations, i);
        json_object *name;

        json_object_object_get_ex(cur_json_relation, "name", &name);
        add_last(rel_names, json_object_get_string(name)); 
    }
}

static void parse_json_node_labels(json_object *json_node, linked_list *labels) {
    size_t labels_size; 
    size_t i; 
    json_object *json_labels; 
    json_object_object_get_ex(json_node, "labels", &json_labels); 

    labels_size = json_object_array_length(json_labels);
    
    for (i=0;i<labels_size; i++) {
        json_object *cur_json_label = json_object_array_get_idx(json_labels, i);
        json_object *name;

        json_object_object_get_ex(cur_json_label, "name", &name);
        add_last(labels, json_object_get_string(name)); 
    }
}

static void parse_json_node_props(json_object *json_node, linked_list *props) {
    size_t props_size; 
    size_t i; 
    json_object *json_props; 
    json_object_object_get_ex(json_node, "props", &json_props); 

    props_size = json_object_array_length(json_props);
    
    for (i=0;i<props_size; i++) {
        property *current_prop = my_alloc(sizeof(property));
        json_object *cur_json_prop = json_object_array_get_idx(json_props, i);
        json_object *key;
        json_object *value;

        json_object_object_get_ex(cur_json_prop, "key", &key);
        json_object_object_get_ex(cur_json_prop, "value", &value);

        bzero(current_prop->key, strlen((current_prop->key)));
        bzero(current_prop->value, strlen((current_prop->value)));

        strcpy(current_prop->key, json_object_get_string(key));
        strcpy(current_prop->value, json_object_get_string(value));
        add_last(props, current_prop); 
    }
}

static void build_json_relations(json_object *jnode, linked_list *relations_list) {
    node *current_relation = relations_list->first;
    json_object *relationsArray = json_object_new_array();
    for (int i = 0; i< relations_list->size; ++i) {
        json_object * jsonRelation = json_object_new_object(); 
        json_object *value = json_object_new_string((char *) current_relation->value);
        json_object_object_add(jsonRelation, "name", value);
        json_object_array_add(relationsArray, jsonRelation); 

        current_relation = current_relation->next; 
    }

    json_object_object_add(jnode, "relations", relationsArray); 
}

static void build_json_node_labels(json_object *jnode, linked_list *labels_list) {  
    node *current_label = labels_list->first;
    json_object *labelsArray = json_object_new_array();
    for (int i = 0; i< labels_list->size; ++i) {
        json_object * jsonLabel = json_object_new_object(); 
        json_object *value = json_object_new_string((char *) current_label->value);
        json_object_object_add(jsonLabel, "name", value);
        json_object_array_add(labelsArray, jsonLabel); 

        current_label = current_label->next; 
    }

    json_object_object_add(jnode, "labels", labelsArray); 
}

static void build_json_node_props(json_object *jnode, linked_list *props_list) {
    json_object *propsArray = json_object_new_array();
    node *current_prop = props_list->first;
    for (int i = 0; i < props_list->size; ++i) {
        property *current = current_prop->value;

        json_object *prop = json_object_new_object();
        json_object *key = json_object_new_string((char *) current->key);
        json_object *value = json_object_new_string((char *) current->value);
        json_object_object_add(prop, "key", key);
        json_object_object_add(prop, "value", value);
        json_object_array_add(propsArray, prop);

        current_prop = current_prop->next;
    }
    json_object_object_add(jnode, "props", propsArray);
}

static void get_node_labels_string(json_object *labels, char *labels_string) {
    bzero(labels_string, strlen(labels_string));
    size_t labels_length = json_object_array_length(labels);

    if(labels_length > 0) {
        size_t i;
        uint32_t offset = 0;

        json_object *label = json_object_array_get_idx(labels, 0);
        json_object *json_name;
        json_object_object_get_ex(label, "name", &json_name);
        char *name = json_object_get_string(json_name);
        
        sprintf(labels_string, "%s", name);
        
        for (i=1; i<labels_length; ++i) {
            label = json_object_array_get_idx(labels, i);
            offset += strlen(name);
            
            json_object_object_get_ex(label, "name", &json_name);
            name = json_object_get_string(json_name);
            
            memcpy(labels_string + offset, ", ", 2);
            offset += 2;
            char label_name_str[strlen((char *) name)];
            bzero(label_name_str, strlen((char *) name) + 1);
            memcpy(label_name_str, (char *) name, strlen((char *) name));
            memcpy(labels_string + offset, label_name_str, strlen(label_name_str));
        }
    }
}

static void get_node_props_string(json_object *props, char *props_string) {
    bzero(props_string, strlen(props_string));
    size_t props_length = json_object_array_length(props);
    
    if (props_length > 0) {
        size_t i;
        uint32_t offset = 0;
        
        json_object *prop = json_object_array_get_idx(props, 0);
        json_object *json_key;
        json_object *json_value;
        
        json_object_object_get_ex(prop, "key", &json_key);
        json_object_object_get_ex(prop, "value", &json_value);
        char *key = json_object_get_string(json_key);
        char *value = json_object_get_string(json_value);
        sprintf(props_string, "%s=%s", key, value);

        for (i=1; i<props_length; ++i) {
            prop = json_object_array_get_idx(props, i);
            offset += strlen((char *) key)+1+strlen((char *) value);

            json_object_object_get_ex(prop, "key", &json_key);
            json_object_object_get_ex(prop, "value", &json_value);

            key = json_object_get_string(json_key);
            value = json_object_get_string(json_value);
            
            memcpy(props_string + offset, ", ", 2);
            offset += 2;
            
            char prop_str[strlen((char *) key)+1+strlen((char *) value)];
            bzero(prop_str, strlen(prop_str));
            sprintf(prop_str, "%s=%s", (char *) key, (char *) value);
            memcpy(props_string + offset, prop_str, strlen(prop_str));
        }
    }
}