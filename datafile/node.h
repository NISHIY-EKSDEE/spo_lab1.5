//
// Created by subhuman on 24.06.2021.
//

#ifndef SPOLAB15_NODE_H
#define SPOLAB15_NODE_H

#include "datafile.h"

cell_ptr *create_node_cell(datafile *data);

void update_node_labels(datafile *data, cell_ptr *node_ptr, cell_ptr *label_ptr);

void update_node_attributes(datafile *data, cell_ptr *node_ptr, cell_ptr *attribute_ptr);

void update_node_relations(datafile *data, cell_ptr *node_ptr, cell_ptr *relation_ptr);

void delete_nodes(datafile *data, linked_list *node_cells, query_info *info);

#endif //SPOLAB15_NODE_H
