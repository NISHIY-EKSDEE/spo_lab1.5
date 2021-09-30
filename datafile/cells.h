#ifndef SPOLAB15_CELLS_H
#define SPOLAB15_CELLS_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    int32_t block_num;
    int16_t offset;
} cell_ptr;//6 bytes

typedef struct {
    cell_ptr prev;
    cell_ptr name;
} label_cell;//12 bytes

typedef struct {
    cell_ptr prev;
    cell_ptr key;
    cell_ptr value;
} attribute_cell;//18 bytes

typedef struct {
    cell_ptr prev;
    cell_ptr name;
    cell_ptr node_b;
} relation_cell;//18 bytes

typedef struct {
    char is_empty;
    cell_ptr last_label;
    cell_ptr last_attribute;
    cell_ptr last_relation;
} node_cell; //19 bytes

#pragma pack(pop)

#endif //SPOLAB15_CELLS_H
