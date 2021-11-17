#include <bits/types/FILE.h>
#include <stdint.h>
#include <stdbool.h>
#include "blocks.h"
#include "../utils/linked_list.h"
#include "../utils/message.h"

#ifndef SPOLAB15_DATAFILE_H
#define SPOLAB15_DATAFILE_H

#include "../utils/my_alloc.h"

typedef struct {
    FILE *file;
    control_block *ctrl_block;
} datafile;

struct relation_info {
    linked_list *labels;
    linked_list *props;
    bool has_relation;
    char rel_name[RELATION_NAME_SIZE];
    linked_list *rel_node_labels;
    linked_list *rel_node_props;
};

typedef struct relation_info relation_info;

relation_info *init_relation_info(query_info *q_info, uint32_t relation_num);

datafile *init_data(char *file_path);

void fill_block(datafile *data, int32_t block_number, void *buffer);

cell_ptr *create_string_cell(datafile *data, char *string);

long match(relation_info *info, datafile *data, linked_list *node_ptr, linked_list *nodes, bool is_node_a);

long match_query_info(query_info *info, datafile *data, linked_list *node_ptr, linked_list *match_results);

long match_with_known_neighbors(relation_info *info, datafile *data, linked_list *node_ptr, linked_list *nodes, linked_list *neighbors_ptr, bool is_node_a);

void update_control_block(datafile *data);

void update_data_block(datafile *data, int32_t block_number, void *block);

void allocate_new_block(datafile *data, TYPE type);

static void add_new_match_result(
        linked_list *node_ptr, linked_list *nodes, linked_list *node_labels, linked_list *node_props,
        int32_t node_block_num, int16_t offset);

#endif //SPOLAB15_DATAFILE_H

