//
// Created by subhuman on 24.06.2021.
//

#ifndef SPOLAB15_LABEL_H
#define SPOLAB15_LABEL_H

#include "datafile.h"

cell_ptr *create_label_cell(datafile *data, cell_ptr *string_cell, cell_ptr *node_cell);

void set_new_labels(datafile *data, linked_list *node_cells, linked_list *changed_labels);

long remove_labels(datafile *data, linked_list *node_cells, linked_list *changed_labels);

bool match_labels(linked_list *matcher_labels, datafile *data, label_cell last_label, linked_list *node_labels);

#endif //SPOLAB15_LABEL_H
