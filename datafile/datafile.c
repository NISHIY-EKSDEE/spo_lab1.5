#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "datafile.h"
#include "label.h"
#include "attribute.h"

datafile *init_data(char *file_path) {
    datafile *data = my_alloc(sizeof(datafile));
    FILE *data_file;
    if (access(file_path, F_OK) == 0) {
        data_file = fopen(file_path, "r+b");
        data->file = data_file;
        data->ctrl_block = my_alloc(sizeof(control_block));
        fill_block(data, 0, data->ctrl_block);
    } else {
        data_file = fopen(file_path, "wb+");
        control_block *ctrl_block = my_alloc(sizeof(control_block));
        bzero(ctrl_block, sizeof(control_block));
        ctrl_block->metadata.type = CONTROL;
        ctrl_block->empty_block = 1;
        ctrl_block->fragmented_label_block = -1;
        ctrl_block->fragmented_relation_block = -1;
        ctrl_block->fragmented_string_block = -1;
        ctrl_block->fragmented_attribute_block = -1;
        ctrl_block->empty_node_number = 0;
        data->ctrl_block = ctrl_block;
        data->file = data_file;
        update_control_block(data);
    }
    return data;
}

void fill_block(datafile *data, int32_t block_number, void *buffer) {
    fseek(data->file, block_number * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, data->file);
}

cell_ptr *create_string_cell(datafile *data, char *string) {
    if (data->ctrl_block->fragmented_string_block == -1) {
        allocate_new_block(data, STRING);
    }
    int16_t str_length = (int16_t) strlen(string);
    int32_t block_number = data->ctrl_block->fragmented_string_block;
    str_block read;
    fill_block(data, data->ctrl_block->fragmented_string_block, &read);
    int16_t empty_fragment_size = 0;

    cell_ptr *ptr = my_alloc(sizeof(cell_ptr));

    memcpy(&empty_fragment_size, read.data + data->ctrl_block->empty_string_offset, sizeof(empty_fragment_size));
    empty_fragment_size = (int16_t) (1020 - data->ctrl_block->empty_string_offset);
    if (empty_fragment_size > str_length) {
        ptr->block_num = data->ctrl_block->fragmented_string_block;
        ptr->offset = data->ctrl_block->empty_string_offset;
        data->ctrl_block->empty_string_offset += str_length + 2;
        memcpy(read.data + ptr->offset, &str_length, 2);
        strcpy(read.data + ptr->offset + 2, string);
    } else {
        int32_t new_block_number = data->ctrl_block->empty_block;
        allocate_new_block(data, STRING);
        data->ctrl_block->fragmented_string_block = new_block_number;
        data->ctrl_block->empty_string_offset = str_length + 2;
        ptr->offset = 0;
        ptr->block_num = new_block_number;
        fill_block(data, new_block_number, &read);
        memcpy(read.data, &str_length, 2);
        strcpy(read.data + 2, string);
    }
    update_data_block(data, ptr->block_num, &read);
    update_control_block(data);
    return ptr;
}

long match(query_info *info, datafile *data, linked_list *node_ptr, linked_list *nodes, bool is_node_a) {
    int32_t node_block_num = 0;
    node *current_node;
    do {
        block read_node = {0};
        fill_block(data, node_block_num, &read_node);
        int16_t nodes_in_block = read_node.metadata.type == CONTROL ? NODES_IN_CONTROL_BLOCK : NODES_IN_BLOCK;
        for (int16_t offset = 0; offset < nodes_in_block; ++offset) {
            bool matching = false;
            node_cell node = {0};
            if (read_node.metadata.type == CONTROL) {
                memcpy(&node, &((control_block *) &read_node)->nodes[offset], sizeof(node_cell));
            } else {
                memcpy(&node, &((node_block *) &read_node)->nodes[offset], sizeof(node_cell));
            }
            if (node.is_empty == 0) {
                if (node.last_label.offset == 0 && node.last_label.block_num == 0) {
                    break;
                } else {
                    continue;
                }
            }
            label_block read_label = {0};
            attribute_block read_attribute = {0};
            fill_block(data, node.last_label.block_num, &read_label);
            fill_block(data, node.last_attribute.block_num, &read_attribute);
            label_cell label = {0};
            attribute_cell attribute = {0};
            if (read_label.metadata.type != CONTROL) {
                memcpy(&label, &read_label.labels[node.last_label.offset], sizeof(label_cell));
            }
            if (read_attribute.metadata.type != CONTROL) {
                memcpy(&attribute, &read_attribute.attributes[node.last_attribute.offset], sizeof(attribute_cell));
            }
            linked_list *node_labels = init_list();
            linked_list *node_props = init_list();
            if (is_node_a) {
                if (match_labels(info->labels, data, label, nodes == NULL ? NULL : node_labels) &&
                    match_attributes(info->props, data, attribute, nodes == NULL ? NULL : node_props)) {
                    matching = true;
                }
                if (info->has_relation) {
                    matching = false;
                    relation_block read_relation = {0};
                    fill_block(data, node.last_relation.block_num, &read_relation);
                    relation_cell relation = {0};
                    if (read_relation.metadata.type != CONTROL) {
                        memcpy(&relation, &read_relation.relations[node.last_relation.offset], sizeof(relation_cell));
                    }
                    if (relation.name.block_num == 0) {
                        free_list(node_labels, false);
                        free_list(node_props, true);
                        continue;
                    }
                    cell_ptr prev;
                    do {
                        cell_ptr string_ptr = relation.name;
                        str_block read_string = {0};
                        fill_block(data, string_ptr.block_num, &read_string);
                        int16_t size = 0;
                        memcpy(&size, &read_string.data[string_ptr.offset], sizeof(int16_t));
                        char *relation_name = my_alloc(size + 1);
                        bzero(relation_name, strlen(relation_name) + 1);
                        strcpy(relation_name, &read_string.data[string_ptr.offset + 2]);
                        relation_name[size] = '\0';
                        if (strcmp(relation_name, info->rel_name) == 0) {
                            block read_node_b = {0};
                            cell_ptr *node_b_ptr = &relation.node_b;
                            fill_block(data, node_b_ptr->block_num, &read_node_b);
                            node_cell node_b = {0};
                            if (read_node_b.metadata.type == CONTROL) {
                                memcpy(&node_b, &((control_block *) &read_node_b)->nodes[node_b_ptr->offset],
                                       sizeof(node_cell));
                            } else {
                                memcpy(&node_b, &((node_block *) &read_node_b)->nodes[node_b_ptr->offset],
                                       sizeof(node_cell));
                            }
                            linked_list *node_b_list = init_list();
                            match(info, data, node_b_list, NULL, false);
                            for (current_node = node_b_list->first; current_node; current_node = current_node->next) {
                                cell_ptr *current_ptr = (cell_ptr *) current_node->value;
                                if (
                                        current_ptr->block_num == node_b_ptr->block_num &&
                                        current_ptr->offset == node_b_ptr->offset
                                        ) {
                                    add_new_match_result(node_ptr, nodes, node_labels, node_props, node_block_num,
                                                         offset);
                                    matching = true;
                                }
                            }
                            free_list(node_b_list, true);
                        }
                        my_free(relation_name);
                        relation_block read_relations = {0};
                        prev = relation.prev;
                        fill_block(data, prev.block_num, &read_relations);
                        memcpy(&relation, &read_relations.relations[prev.offset], sizeof(relation_cell));
                    } while (!(prev.block_num == 0 && prev.offset == 0));
                } else {
                    if(matching) add_new_match_result(node_ptr, nodes, node_labels, node_props, node_block_num, offset);
                }
            } else {
                if (match_labels(info->rel_node_labels, data, label, nodes == NULL ? NULL : node_labels) &&
                    match_attributes(info->rel_node_props, data, attribute, nodes == NULL ? NULL : node_props)) {
                    add_new_match_result(node_ptr, NULL, node_labels, node_props, node_block_num, offset);
                    matching = true;
                }
            }
            if (!matching) {
                free_list(node_labels, false);
                free_list(node_props, true);
            }
        }
        node_block_num = read_node.metadata.type == CONTROL ?
                         ((control_block *) &read_node)->prev_node_block
                                                            :
                         ((node_block *) &read_node)->prev_block;
    } while (node_block_num != 0);
    return node_ptr->size;
}

void allocate_new_block(datafile *data, TYPE type) {
    fseek(data->file, data->ctrl_block->empty_block * BLOCK_SIZE, SEEK_SET);
    block new_block = {0};
    new_block.metadata.type = type;
    int32_t block_number = data->ctrl_block->empty_block;
    update_data_block(data, block_number, &new_block);
    switch (type) {
        case CONTROL:
            break;
        case NODE:
            data->ctrl_block->fragmented_node_block = block_number;
            if (block_number == 1) {
                data->ctrl_block->prev_node_block = block_number;
            } else {
                node_block block = {0};
                block.metadata.type = NODE;
                block.prev_block = data->ctrl_block->prev_node_block;
                update_data_block(data, block_number, &block);
                data->ctrl_block->prev_node_block = block_number;
                update_control_block(data);
            }
            break;
        case LABEL:
            data->ctrl_block->fragmented_label_block = block_number;
            break;
        case ATTRIBUTE:
            data->ctrl_block->fragmented_attribute_block = block_number;
            break;
        case RELATION:
            data->ctrl_block->fragmented_relation_block = block_number;
            break;
        case STRING:
            data->ctrl_block->fragmented_string_block = block_number;
            break;
        default:
            break;
    }
    data->ctrl_block->empty_block = block_number + 1;
    update_control_block(data);
}

void update_control_block(datafile *data) {
    update_data_block(data, 0, data->ctrl_block);
}

void update_data_block(datafile *data, int32_t block_number, void *block) {
    fseek(data->file, block_number * BLOCK_SIZE, SEEK_SET);
    fwrite(block, BLOCK_SIZE, 1, data->file);
}

static void add_new_match_result(
        linked_list *node_ptr, linked_list *nodes, linked_list *node_labels, linked_list *node_props,
        int32_t node_block_num, int16_t offset) {
    cell_ptr *ptr = my_alloc(sizeof(cell_ptr));
    ptr->block_num = node_block_num;
    ptr->offset = offset;
    add_last(node_ptr, ptr);
    if (nodes == NULL) {
        free_list(node_labels, false);
        free_list(node_props, true);
    } else {
        match_result *match = my_alloc(sizeof(match_result));
        match->labels = node_labels;
        match->props = node_props;
        add_last(nodes, match);
    }
}