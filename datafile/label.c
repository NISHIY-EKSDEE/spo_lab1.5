//
// Created by subhuman on 24.06.2021.
//

#include <string.h>
#include "label.h"
#include "node.h"

cell_ptr *create_label_cell(datafile *data, cell_ptr *string_cell, cell_ptr *node_cell) {
    if (data->ctrl_block->fragmented_label_block == -1) {
        allocate_new_block(data, LABEL);
    }
    label_cell new_cell = {0};
    memcpy(&new_cell.name, string_cell, sizeof(cell_ptr));

    block read_node = {0};
    fill_block(data, node_cell->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&new_cell.prev, &((control_block *) &read_node)->nodes[node_cell->offset].last_label, sizeof(cell_ptr));
    } else {
        memcpy(&new_cell.prev, &((node_block *) &read_node)->nodes[node_cell->offset].last_label, sizeof(cell_ptr));
    }

    cell_ptr *ptr = my_alloc(sizeof(cell_ptr));
    ptr->block_num = data->ctrl_block->fragmented_label_block;
    ptr->offset = data->ctrl_block->empty_label_number;
    int32_t block_number = data->ctrl_block->fragmented_label_block;
    label_block read;
    fill_block(data, data->ctrl_block->fragmented_label_block, &read);

    label_cell old_cell = {0};
    memcpy(&old_cell, &read.labels[data->ctrl_block->empty_label_number], sizeof(label_cell));
    memcpy(&read.labels[data->ctrl_block->empty_label_number], &new_cell, sizeof(label_cell));

    int16_t new_label_offset;
    if (old_cell.prev.block_num == 0 && old_cell.prev.offset == 0) {
        new_label_offset = (int16_t) (data->ctrl_block->empty_label_number + 1);
        if (new_label_offset > LABELS_IN_BLOCK - 1) {
            new_label_offset = 0;
            data->ctrl_block->fragmented_label_block = -1;
        }
        data->ctrl_block->empty_label_number = new_label_offset;
    } else {
        data->ctrl_block->fragmented_label_block = old_cell.prev.block_num;
        data->ctrl_block->empty_label_number = old_cell.prev.offset;
    }
    update_data_block(data, block_number, &read);
    update_control_block(data);
    return ptr;
}

bool match_labels(linked_list *matcher_labels, datafile *data, label_cell last_label, linked_list *node_labels) {
    label_cell label = last_label;
    cell_ptr prev;
    linked_list *labels = init_list();
    for (node *current_matcher_label = matcher_labels->first; current_matcher_label; current_matcher_label = current_matcher_label->next) {
        add_last(labels, current_matcher_label->value);
    }
    if (label.name.block_num == 0){
        free_list(labels, false);
        return labels->size == 0;
    }
    do {
        cell_ptr string_ptr = label.name;
        str_block read_string = {0};
        fill_block(data, string_ptr.block_num, &read_string);
        int16_t size = 0;
        memcpy(&size, &read_string.data[string_ptr.offset], sizeof(int16_t));
        char *label_name =  malloc(size + 1);
        bzero(label_name, strlen(label_name) + 1);
        strcpy(label_name, &read_string.data[string_ptr.offset + 2]);
        label_name[size] = '\0';
        remove_element(by_value, labels, label_name, NULL);
        if (node_labels != NULL) {
            add_last(node_labels, label_name);
        }
        label_block read_labels = {0};
        prev = label.prev;
        fill_block(data, prev.block_num, &read_labels);
        memcpy(&label, &read_labels.labels[prev.offset], sizeof(label_cell));
    } while (!(prev.block_num == 0 && prev.offset == 0));
    free_list(labels, false);
    return labels->size == 0;
}

void set_new_labels(datafile *data, linked_list *node_cells, linked_list *changed_labels) {
    node *label;
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        cell_ptr *node_ptr = current_node->value;
        for (label = changed_labels->first; label; label = label->next) {
            cell_ptr *string_ptr = create_string_cell(data, label->value);
            cell_ptr *label_ptr = create_label_cell(data, string_ptr, node_ptr);
            update_node_labels(data, node_ptr, label_ptr);
        }
    }
}

long remove_labels(datafile *data, linked_list *node_cells, linked_list *changed_labels) {
    long number = 0;
    node *query_label;
    label_block read_label;
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        bool has_changed = false;
        block read_node = {0};
        cell_ptr *node_ptr = current_node->value;
        fill_block(data, node_ptr->block_num, &read_node);
        node_cell node = {0};
        if (read_node.metadata.type == CONTROL) {
            memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        } else {
            memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        }
        linked_list *labels = init_list();
        if (changed_labels != NULL) {
            for (query_label = changed_labels->first; query_label; query_label = query_label->next) {
                add_last(labels, query_label->value);
            }
        }
        int32_t block_number = node.last_label.block_num;
        int16_t offset = node.last_label.offset;
        cell_ptr prev = {0};
        memcpy(&prev, node_ptr, sizeof(cell_ptr));
        do {
            bzero(&read_label, BLOCK_SIZE);
            fill_block(data, block_number, &read_label);
            if (read_label.metadata.type == CONTROL) break;
            cell_ptr current = {0};
            current.block_num = block_number;
            current.offset = offset;
            label_cell old_cell = {0};
            label_cell new_cell = {0};
            memcpy(&old_cell, &read_label.labels[offset], sizeof(label_cell));
            cell_ptr string_ptr = old_cell.name;
            str_block read_string = {0};
            fill_block(data, string_ptr.block_num, &read_string);
            int16_t size = 0;
            memcpy(&size, &read_string.data[string_ptr.offset], sizeof(int16_t));
            char *label_name = my_alloc(size + 1);
            bzero(label_name, strlen(label_name) + 1);
            strcpy(label_name, &read_string.data[string_ptr.offset + 2]);
            label_name[size] = '\0';
            if (changed_labels == NULL || find_element(by_value, labels, label_name, NULL) != NULL) {
                remove_element(by_value, labels, label_name, NULL);
                bzero(&new_cell, sizeof(label_cell));
                block updated = {0};
                fill_block(data, prev.block_num, &updated);
                if (updated.metadata.type == CONTROL) {
                    memcpy(&node.last_label, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((control_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                    update_data_block(data, node_ptr->block_num, &updated);
                    fill_block(data, 0, data->ctrl_block);
                } else if (updated.metadata.type == NODE) {
                    memcpy(&node.last_label, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((node_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                    update_data_block(data, node_ptr->block_num, &updated);
                } else {
                    label_cell prev_label = ((label_block *) &updated)->labels[prev.offset];
                    memcpy(&prev_label.prev, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((label_block *) &updated)->labels[prev.offset], &prev_label, sizeof(label_cell));
                    update_data_block(data, prev.block_num, &updated);
                }
                bzero(&read_label, BLOCK_SIZE);
                fill_block(data, block_number, &read_label);
                memcpy(&new_cell.prev.block_num, &data->ctrl_block->fragmented_label_block, sizeof(int32_t));
                memcpy(&new_cell.prev.offset, &data->ctrl_block->empty_label_number, sizeof(int16_t));
                memcpy(&((label_block *) &read_label)->labels[offset], &new_cell, sizeof(label_cell));
                update_data_block(data, block_number, &read_label);
                memcpy(&data->ctrl_block->fragmented_label_block, &block_number, sizeof(int32_t));
                memcpy(&data->ctrl_block->empty_label_number, &offset, sizeof(int16_t));
                update_control_block(data);
                if (!has_changed) number++;
                has_changed = true;
            }
            memcpy(&prev, &current, sizeof(cell_ptr));
            block_number = old_cell.prev.block_num;
            offset = old_cell.prev.offset;
        } while (read_label.metadata.type != CONTROL);
    }
    return number;
}