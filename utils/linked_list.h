#ifndef SPOLAB3_LINKED_LIST_H
#define SPOLAB3_LINKED_LIST_H

#include <stdint.h>
#include <stdbool.h>

struct node {
    struct node *prev;
    void *value;
    struct node *next;
};

typedef struct node node;

typedef struct {
    struct node *first;
    struct node *last;
    uint32_t size;

    void (*free_value_fun)(void *value);
} linked_list;

void *get_element(linked_list *ptr, uint32_t index);

uint32_t add_first(linked_list *ptr, void *element);

uint32_t add_last(linked_list *ptr, void *element);

bool by_value(void *value, char *to_find, char *second_argument);

bool by_key(void *value, char *key, char *second_value);

linked_list *init_list();

void free_list(linked_list *ptr, bool is_alloc_value);

void remove_element(bool (*by)(void *, char *, char *), linked_list *ptr, char *first_to_find, char *second_to_find);

void *find_element(bool (*by)(void *, char *, char *), linked_list *ptr, char *first_to_find, char *second_to_find);

uint16_t get_last_n(linked_list *ptr, void **buffer, uint16_t buffer_size, bool(*filter)(void*, char*to_filter), char*to_filter);

#endif //SPOLAB3_LINKED_LIST_H
