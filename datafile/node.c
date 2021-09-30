//
// Created by subhuman on 24.06.2021.
//

#include <string.h>
#include "node.h"
#include "attribute.h"
#include "label.h"
#include "relation.h"

cell_ptr *create_node_cell(datafile *data) {
    if (data->ctrl_block->fragmented_node_block == -1) {
        allocate_new_block(data, NODE);
    }
    node_cell new_cell = {0};
    new_cell.is_empty = 1;
    cell_ptr *ptr = my_alloc(sizeof(cell_ptr));

    ptr->block_num = data->ctrl_block->fragmented_node_block;
    ptr->offset = data->ctrl_block->empty_node_number;

    int32_t block_number = data->ctrl_block->fragmented_node_block;
    block read_node;
    fill_block(data, data->ctrl_block->fragmented_node_block, &read_node);
    node_cell old_cell;
    if (read_node.metadata.type == CONTROL) {
        old_cell = ((control_block *) &read_node)->nodes[data->ctrl_block->empty_node_number];
        memcpy(&((control_block *) &read_node)->nodes[data->ctrl_block->empty_node_number], &new_cell,
               sizeof(node_cell));
        update_data_block(data, block_number, &read_node);
        fill_block(data, 0, data->ctrl_block);
    } else {
        old_cell = ((node_block *) &read_node)->nodes[data->ctrl_block->empty_node_number];
        memcpy(&((node_block *) &read_node)->nodes[data->ctrl_block->empty_node_number], &new_cell, sizeof(node_cell));
        update_data_block(data, block_number, &read_node);
    }

    int16_t new_node_offset;
    if (old_cell.last_label.block_num == 0 && old_cell.last_label.offset == 0) {
        new_node_offset = (int16_t) (data->ctrl_block->empty_node_number + 1);
        if (new_node_offset > NODES_IN_CONTROL_BLOCK - 1 && ptr->block_num == 0 ||
            new_node_offset > NODES_IN_BLOCK - 1 && ptr->block_num != 0) {
            new_node_offset = 0;
            data->ctrl_block->fragmented_node_block = -1;
        }
        data->ctrl_block->empty_node_number = new_node_offset;
    } else {
        data->ctrl_block->fragmented_node_block = old_cell.last_label.block_num;
        data->ctrl_block->empty_node_number = old_cell.last_label.offset;
    }
    update_control_block(data);
    return ptr;
}

void update_node_labels(datafile *data, cell_ptr *node_ptr, cell_ptr *label_ptr) {
    node_cell node = {0};
    block read_node;
    fill_block(data, node_ptr->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_label, label_ptr, sizeof(cell_ptr));
        memcpy(&((control_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    } else {
        memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_label, label_ptr, sizeof(cell_ptr));
        memcpy(&((node_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    }
    update_data_block(data, node_ptr->block_num, &read_node);
    fill_block(data, 0, data->ctrl_block);
}

void update_node_attributes(datafile *data, cell_ptr *node_ptr, cell_ptr *attribute_ptr) {
    node_cell node = {0};
    block read_node;
    fill_block(data, node_ptr->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_attribute, attribute_ptr, sizeof(cell_ptr));
        memcpy(&((control_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    } else {
        memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_attribute, attribute_ptr, sizeof(cell_ptr));
        memcpy(&((node_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    }
    update_data_block(data, node_ptr->block_num, &read_node);
    fill_block(data, 0, data->ctrl_block);
}

void update_node_relations(datafile *data, cell_ptr *node_ptr, cell_ptr *relation_ptr) {
    node_cell node = {0};
    block read_node;
    fill_block(data, node_ptr->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_relation, relation_ptr, sizeof(cell_ptr));
        memcpy(&((control_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    } else {
        memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_relation, relation_ptr, sizeof(cell_ptr));
        memcpy(&((node_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    }
    update_data_block(data, node_ptr->block_num, &read_node);
    fill_block(data, 0, data->ctrl_block);
}

void delete_nodes(datafile *data, linked_list *node_cells, query_info *info) {
    remove_labels(data, node_cells, NULL);
    remove_attributes(data, node_cells, NULL);
    delete_relations(data, info, node_cells, NULL);
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        block read_node = {0};
        cell_ptr *node_ptr = current_node->value;
        fill_block(data, node_ptr->block_num, &read_node);
        node_cell node = {0};
        if (read_node.metadata.type == CONTROL) {
            memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
            node.is_empty = 0;
            memcpy(&node.last_label.block_num, &data->ctrl_block->fragmented_node_block, sizeof(int32_t));
            memcpy(&node.last_label.offset, &data->ctrl_block->empty_node_number, sizeof(int16_t));
            memcpy(&((control_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
        } else {
            memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
            node.is_empty = 0;
            memcpy(&node.last_label.block_num, &data->ctrl_block->fragmented_node_block, sizeof(int32_t));
            memcpy(&node.last_label.offset, &data->ctrl_block->empty_node_number, sizeof(int16_t));
            memcpy(&((node_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
        }
        update_data_block(data, node_ptr->block_num, &read_node);
        fill_block(data, 0, data->ctrl_block);
        memcpy(&data->ctrl_block->fragmented_node_block, &node_ptr->block_num, sizeof(int32_t));
        memcpy(&data->ctrl_block->empty_node_number, &node_ptr->offset, sizeof(int16_t));
        update_control_block(data);
    }
}
