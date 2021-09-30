#include <string.h>
#include "relation.h"
#include "cells.h"
#include "datafile.h"

cell_ptr *create_relation_cell(datafile *data, cell_ptr *string_cell, cell_ptr *first_node_cell, cell_ptr *second_node_cell) {
    if (data->ctrl_block->fragmented_relation_block == -1) {
        allocate_new_block(data, RELATION);
    }
    relation_cell new_cell = {0};
    memcpy(&new_cell.name, string_cell, sizeof(cell_ptr));
    memcpy(&new_cell.node_b, second_node_cell, sizeof(cell_ptr));

    block read_node = {0};
    fill_block(data, first_node_cell->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&new_cell.prev, &((control_block *) &read_node)->nodes[first_node_cell->offset].last_relation, sizeof(cell_ptr));
    } else {
        memcpy(&new_cell.prev, &((node_block *) &read_node)->nodes[first_node_cell->offset].last_relation, sizeof(cell_ptr));
    }

    cell_ptr *ptr = my_alloc(sizeof(cell_ptr));
    int32_t block_number = data->ctrl_block->fragmented_relation_block;
    ptr->block_num = block_number;
    ptr->offset = data->ctrl_block->empty_relation_number;

    relation_block read_relation;
    fill_block(data, block_number, &read_relation);

    relation_cell old_cell = {0};
    memcpy(&old_cell, &read_relation.relations[ptr->offset], sizeof(relation_cell));
    memcpy(&read_relation.relations[ptr->offset], &new_cell, sizeof(relation_cell));

    int16_t new_relation_offset;
    if (old_cell.prev.block_num == 0 && old_cell.prev.offset == 0) {
        new_relation_offset = (int16_t) (data->ctrl_block->empty_relation_number + 1);
        if (new_relation_offset > RELATIONS_IN_BLOCK - 1) {
            new_relation_offset = 0;
            data->ctrl_block->fragmented_relation_block = -1;
        }
        data->ctrl_block->empty_relation_number = new_relation_offset;
    } else {
        data->ctrl_block->fragmented_relation_block = old_cell.prev.block_num;
        data->ctrl_block->empty_relation_number = old_cell.prev.offset;
    }
    update_data_block(data, block_number, &read_relation);
    update_control_block(data);
    return ptr;
}

long delete_relations(datafile *data, query_info* info, linked_list *node_cells, char *rel_name) {
    long number = 0;
    relation_block read_relation;
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        block read_node = {0};
        cell_ptr *node_ptr = current_node->value;
        fill_block(data, node_ptr->block_num, &read_node);
        node_cell node = {0};
        if (read_node.metadata.type == CONTROL) {
            memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        } else {
            memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        }
        int32_t block_number = node.last_relation.block_num;
        int16_t offset = node.last_relation.offset;
        cell_ptr prev = {0};
        memcpy(&prev, node_ptr, sizeof(cell_ptr));
        do {
            bzero(&read_relation, BLOCK_SIZE);
            fill_block(data, block_number, &read_relation);
            if (read_relation.metadata.type == CONTROL) break;
            cell_ptr current = {0};
            current.block_num = block_number;
            current.offset = offset;
            relation_cell old_cell = {0};
            relation_cell new_cell = {0};
            memcpy(&old_cell, &read_relation.relations[offset], sizeof(relation_cell));
            cell_ptr string_ptr = old_cell.name;
            str_block read_string = {0};
            fill_block(data, string_ptr.block_num, &read_string);
            int16_t size = 0;
            memcpy(&size, &read_string.data[string_ptr.offset], sizeof(int16_t));
            char *relation_name = my_alloc(size + 1);
            bzero(relation_name, strlen(relation_name) + 1);
            strcpy(relation_name, &read_string.data[string_ptr.offset + 2]);
            relation_name[size] = '\0';
            if (rel_name == NULL || strcmp(relation_name, rel_name) == 0) {
                block read_node_b = {0};
                cell_ptr *node_b_ptr = &old_cell.node_b;
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
                        bzero(&new_cell, sizeof(relation_cell));
                        block updated = {0};
                        fill_block(data, prev.block_num, &updated);
                        if (updated.metadata.type == CONTROL) {
                            memcpy(&node.last_relation, &old_cell.prev, sizeof(cell_ptr));
                            memcpy(&((control_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                            update_data_block(data, node_ptr->block_num, &updated);
                            fill_block(data, 0, data->ctrl_block);
                        } else if (updated.metadata.type == NODE) {
                            memcpy(&node.last_relation, &old_cell.prev, sizeof(cell_ptr));
                            memcpy(&((node_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                            update_data_block(data, node_ptr->block_num, &updated);
                        } else {
                            relation_cell prev_relation = ((relation_block *) &updated)->relations[prev.offset];
                            memcpy(&prev_relation.prev, &old_cell.prev, sizeof(cell_ptr));
                            memcpy(&((relation_block *) &updated)->relations[prev.offset], &prev_relation, sizeof(relation_cell));
                            update_data_block(data, prev.block_num, &updated);
                        }
                        bzero(&read_relation, BLOCK_SIZE);
                        fill_block(data, block_number, &read_relation);
                        memcpy(&new_cell.prev.block_num, &data->ctrl_block->fragmented_relation_block, sizeof(int32_t));
                        memcpy(&new_cell.prev.offset, &data->ctrl_block->empty_relation_number, sizeof(int16_t));
                        memcpy(&((relation_block *) &read_relation)->relations[offset], &new_cell, sizeof(label_cell));
                        update_data_block(data, block_number, &read_relation);
                        memcpy(&data->ctrl_block->fragmented_relation_block, &block_number, sizeof(int32_t));
                        memcpy(&data->ctrl_block->empty_relation_number, &offset, sizeof(int16_t));
                        update_control_block(data);
                        number++;
                    }
                }
                free_list(node_b_list, true);
            }
            memcpy(&prev, &current, sizeof(cell_ptr));
            block_number = old_cell.prev.block_num;
            offset = old_cell.prev.offset;
        } while (read_relation.metadata.type != CONTROL);
    }
    return number;
}