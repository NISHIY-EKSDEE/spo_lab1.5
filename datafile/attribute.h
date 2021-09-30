#ifndef SPOLAB15_ATTRIBUTE_H
#define SPOLAB15_ATTRIBUTE_H

#include "datafile.h"

cell_ptr *create_attribute_cell(datafile *data, cell_ptr *key_cell, cell_ptr *value_cell, cell_ptr *node_cell);

void set_new_attributes(datafile *data, linked_list *node_cells, linked_list *changed_props);

long remove_attributes(datafile *data, linked_list *node_cells, linked_list *changed_props);

bool match_attributes(linked_list *matcher_attributes, datafile *data, attribute_cell last_attribute, linked_list *node_attributes);

cell_ptr *find_node_attribute(datafile *data, cell_ptr *node_ptr, char *matcher_key);

#endif //SPOLAB15_ATTRIBUTE_H
