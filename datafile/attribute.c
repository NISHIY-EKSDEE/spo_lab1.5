#include <string.h>
#include "attribute.h"
#include "node.h"

cell_ptr *create_attribute_cell(datafile *data, cell_ptr *key_cell, cell_ptr *value_cell, cell_ptr *node_cell) {
    if (data->ctrl_block->fragmented_attribute_block == -1) {
        allocate_new_block(data, ATTRIBUTE);
    }
    attribute_cell new_cell = {0};
    memcpy(&new_cell.key, key_cell, sizeof(cell_ptr));
    memcpy(&new_cell.value, value_cell, sizeof(cell_ptr));

    block read_node = {0};
    fill_block(data, node_cell->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&new_cell.prev, &((control_block *) &read_node)->nodes[node_cell->offset].last_attribute,
               sizeof(cell_ptr));
    } else {
        memcpy(&new_cell.prev, &((node_block *) &read_node)->nodes[node_cell->offset].last_attribute, sizeof(cell_ptr));
    }

    cell_ptr *ptr = my_alloc(sizeof(cell_ptr));
    ptr->block_num = data->ctrl_block->fragmented_attribute_block;
    ptr->offset = data->ctrl_block->empty_attribute_number;
    int32_t block_number = data->ctrl_block->fragmented_attribute_block;
    attribute_block read_attribute;
    fill_block(data, data->ctrl_block->fragmented_attribute_block, &read_attribute);

    attribute_cell old_cell = {0};
    memcpy(&old_cell, &read_attribute.attributes[data->ctrl_block->empty_attribute_number], sizeof(attribute_cell));
    memcpy(&read_attribute.attributes[data->ctrl_block->empty_attribute_number], &new_cell, sizeof(attribute_cell));

    int16_t new_attribute_offset;
    if (old_cell.prev.block_num == 0 && old_cell.prev.offset == 0) {
        new_attribute_offset = (int16_t) (data->ctrl_block->empty_attribute_number + 1);
        if (new_attribute_offset > ATTRIBUTES_IN_BLOCK - 1) {
            new_attribute_offset = 0;
            data->ctrl_block->fragmented_attribute_block = -1;
        }
        data->ctrl_block->empty_attribute_number = new_attribute_offset;
    } else {
        data->ctrl_block->fragmented_attribute_block = old_cell.prev.block_num;
        data->ctrl_block->empty_attribute_number = old_cell.prev.offset;
    }
    update_data_block(data, block_number, &read_attribute);
    update_control_block(data);
    return ptr;
}

bool match_attributes(linked_list *matcher_attributes, datafile *data, attribute_cell last_attribute,
                             linked_list *node_attributes) {
    attribute_cell attribute = last_attribute;
    cell_ptr prev;
    linked_list *attributes = init_list();
    for (node *current_matcher_attribute = matcher_attributes->first; current_matcher_attribute; current_matcher_attribute = current_matcher_attribute->next) {
        property *prop = my_alloc(sizeof(property));
        bzero(prop, sizeof(property));
        strcpy(prop->key, ((property *) current_matcher_attribute->value)->key);
        strcpy(prop->value, ((property *) current_matcher_attribute->value)->value);
        add_last(attributes, prop);
    }
    if (attribute.key.block_num == 0 || attribute.value.block_num == 0) {
        free_list(attributes, true);
        return attributes->size == 0;
    }
    do {
        cell_ptr key_ptr = attribute.key;
        cell_ptr value_ptr = attribute.value;
        str_block read_key = {0};
        str_block read_value = {0};
        fill_block(data, key_ptr.block_num, &read_key);
        fill_block(data, value_ptr.block_num, &read_value);
        int16_t key_size = 0;
        int16_t value_size = 0;
        memcpy(&key_size, &read_key.data[key_ptr.offset], sizeof(int16_t));
        memcpy(&value_size, &read_value.data[value_ptr.offset], sizeof(int16_t));
        char attr_key [key_size + 1];
        char attr_value [value_size + 1];
        bzero(attr_key, strlen(attr_key) + 1);
        bzero(attr_value, strlen(attr_value) + 1);
        strcpy(attr_key, &read_key.data[key_ptr.offset + 2]);
        strcpy(attr_value, &read_value.data[value_ptr.offset + 2]);
        attr_key[key_size] = '\0';
        attr_value[value_size] = '\0';
        if (node_attributes != NULL) {
            property *prop = my_alloc(sizeof(property));
            bzero(prop, sizeof(property));
            strcpy(prop->key, attr_key);
            strcpy(prop->value, attr_value);
            add_last(node_attributes, prop);
        }
        remove_element(by_property_values, attributes, attr_key, attr_value);
        attribute_block read_attributes = {0};
        prev = attribute.prev;
        fill_block(data, prev.block_num, &read_attributes);
        memcpy(&attribute, &read_attributes.attributes[prev.offset], sizeof(attribute_cell));
    } while (!(prev.block_num == 0 && prev.offset == 0));
    free_list(attributes, true);
    return attributes->size == 0;
}

void set_new_attributes(datafile *data, linked_list *node_cells, linked_list *changed_props) {
    node *prop;
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        cell_ptr *node_ptr = current_node->value;
        for (prop = changed_props->first; prop; prop = prop->next) {
            cell_ptr *found = find_node_attribute(data, node_ptr, ((property *) prop->value)->key);
            if (found == NULL) {
                cell_ptr *key_ptr = create_string_cell(data, ((property *) prop->value)->key);
                cell_ptr *value_ptr = create_string_cell(data, ((property *) prop->value)->value);
                cell_ptr *attr_ptr = create_attribute_cell(data, key_ptr, value_ptr, node_ptr);
                update_node_attributes(data, node_ptr, attr_ptr);
            } else {
                attribute_block read_attribute;
                fill_block(data, found->block_num, &read_attribute);
                attribute_cell attribute = {0};
                memcpy(&attribute, &read_attribute.attributes[found->offset], sizeof(attribute_cell));
                cell_ptr *value_ptr = create_string_cell(data, ((property *) prop->value)->value);
                memcpy(&attribute.value, value_ptr, sizeof(cell_ptr));
                memcpy(&read_attribute.attributes[found->offset], &attribute, sizeof(attribute_cell));
                update_data_block(data, found->block_num, &read_attribute);
            }
        }
    }
}

cell_ptr *find_node_attribute(datafile *data, cell_ptr *node_ptr, char *matcher_key) {
    cell_ptr *result = NULL;
    block read_node = {0};
    fill_block(data, node_ptr->block_num, &read_node);
    node_cell node = {0};
    if (read_node.metadata.type == CONTROL) {
        memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
    } else {
        memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
    }
    cell_ptr attribute_ptr = node.last_attribute;
    if (attribute_ptr.block_num == 0) return NULL;
    int32_t block_number = attribute_ptr.block_num;
    int16_t offset = attribute_ptr.offset;
    cell_ptr *next = my_alloc(sizeof(cell_ptr));
    memcpy(next, node_ptr, sizeof(cell_ptr));
    do {
        attribute_block read_attribute;
        fill_block(data, block_number, &read_attribute);
        attribute_cell *attribute = my_alloc(sizeof(attribute_cell));
        memcpy(attribute, &read_attribute.attributes[offset], sizeof(attribute_cell));
        cell_ptr key_ptr = attribute->key;
        str_block read_string = {0};
        fill_block(data, key_ptr.block_num, &read_string);
        int16_t size = 0;
        memcpy(&size, &read_string.data[key_ptr.offset], sizeof(int16_t));
        char *key = my_alloc(size + 1);
        bzero(key, strlen(key) + 1);
        strcpy(key, &read_string.data[key_ptr.offset + 2]);
        key[size] = '\0';
        if (strcmp(key, matcher_key) == 0) {
            cell_ptr *ptr = my_alloc(sizeof(cell_ptr));
            memcpy(&ptr->block_num, &block_number, sizeof(int32_t));
            memcpy(&ptr->offset, &offset, sizeof(int16_t));
            return ptr;
        } else {
            memcpy(&next->block_num, &block_number, sizeof(int32_t));
            memcpy(&next->offset, &offset, sizeof(int16_t));
            block_number = attribute->prev.block_num;
            offset = attribute->prev.offset;
        }
    } while (block_number != 0);
    return result;
}

long remove_attributes(datafile *data, linked_list *node_cells, linked_list *changed_props) {
    long number = 0;
    node *query_attr;
    attribute_block read_attr = {0};
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
        linked_list *attributes = init_list();
        if (changed_props != NULL) {
            for (query_attr = changed_props->first; query_attr; query_attr = query_attr->next) {
                property *prop = my_alloc(sizeof(property));
                bzero(prop, sizeof(property));
                strcpy(prop->key, ((property *) query_attr->value)->key);
                strcpy(prop->value, ((property *) query_attr->value)->value);
                add_last(attributes, prop);
            }
        }
        int32_t block_number = node.last_attribute.block_num;
        int16_t offset = node.last_attribute.offset;
        cell_ptr prev = {0};
        memcpy(&prev, node_ptr, sizeof(cell_ptr));
        do {
            bzero(&read_attr, BLOCK_SIZE);
            fill_block(data, block_number, &read_attr);
            if (read_attr.metadata.type == CONTROL) break;
            cell_ptr current = {0};
            current.block_num = block_number;
            current.offset = offset;
            attribute_cell old_cell = {0};
            attribute_cell new_cell = {0};
            memcpy(&old_cell, &read_attr.attributes[offset], sizeof(attribute_cell));
            cell_ptr key_ptr = old_cell.key;
            str_block read_key = {0};
            fill_block(data, key_ptr.block_num, &read_key);
            int16_t size = 0;
            memcpy(&size, &read_key.data[key_ptr.offset], sizeof(int16_t));
            char *key = my_alloc(size + 1);
            bzero(key, strlen(key) + 1);
            strcpy(key, &read_key.data[key_ptr.offset + 2]);
            key[size] = '\0';
            if (changed_props == NULL || find_element(by_key, attributes, key, NULL) != NULL) {
                remove_element(by_key, attributes, key, NULL);
                bzero(&new_cell, sizeof(attribute_cell));
                block updated = {0};
                fill_block(data, prev.block_num, &updated);
                if (updated.metadata.type == CONTROL) {
                    memcpy(&node.last_attribute, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((control_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                    update_data_block(data, node_ptr->block_num, &updated);
                    fill_block(data, 0, data->ctrl_block);
                } else if (updated.metadata.type == NODE) {
                    memcpy(&node.last_attribute, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((node_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                    update_data_block(data, node_ptr->block_num, &updated);
                } else {
                    attribute_cell prev_attr = ((attribute_block *) &updated)->attributes[prev.offset];
                    memcpy(&prev_attr.prev, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((attribute_block *) &updated)->attributes[prev.offset], &prev_attr,
                           sizeof(attribute_cell));
                    update_data_block(data, prev.block_num, &updated);
                }
                bzero(&read_attr, BLOCK_SIZE);
                fill_block(data, block_number, &read_attr);
                memcpy(&new_cell.prev.block_num, &data->ctrl_block->fragmented_attribute_block, sizeof(int32_t));
                memcpy(&new_cell.prev.offset, &data->ctrl_block->empty_attribute_number, sizeof(int16_t));
                memcpy(&((attribute_block *) &read_attr)->attributes[offset], &new_cell, sizeof(attribute_cell));
                update_data_block(data, block_number, &read_attr);
                memcpy(&data->ctrl_block->fragmented_attribute_block, &block_number, sizeof(int32_t));
                memcpy(&data->ctrl_block->empty_attribute_number, &offset, sizeof(int16_t));
                update_control_block(data);
                if (!has_changed) number++;
                has_changed = true;
            }
            memcpy(&prev, &current, sizeof(cell_ptr));
            block_number = old_cell.prev.block_num;
            offset = old_cell.prev.offset;
        } while (read_attr.metadata.type != CONTROL);
    }
    return number;
}