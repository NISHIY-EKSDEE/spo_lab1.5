//
// Created by subhuman on 25.06.2021.
//

#include <malloc.h>
#include "my_alloc.h"

typedef struct {
    size_t size;
    void *body;
} _head;

static unsigned long long _my_alloc_max;
static unsigned long long _my_alloc_current;
static unsigned long long _my_alloc_all;

void init_alloc() {
    _my_alloc_max = 0;
    _my_alloc_current = 0;
    _my_alloc_all = 0;
}

void *my_alloc(size_t size) {
    _head *ptr = malloc(size + sizeof(_head) + 16);

    _my_alloc_all += size;
    _my_alloc_current += size;

    if (_my_alloc_max < _my_alloc_current) {
        _my_alloc_max = _my_alloc_current;
    }

    ptr->size = size;
    return &ptr->body;
}

void my_free(void *ptr) {
    _head a;

    _head *head = (char *) ptr - ((char *) &a.body - (char *) &a);

    _my_alloc_current -= head->size;
    free(head);
}

unsigned long long get_max() {
    return _my_alloc_max;
}

unsigned long long get_all() {
    return _my_alloc_all;
}

unsigned long long get_current() {
    return _my_alloc_current;
}