//
// Created by subhuman on 25.06.2021.
//

#ifndef SPOLAB15_MY_ALLOC_H
#define SPOLAB15_MY_ALLOC_H

#include <stddef.h>
#include "my_alloc.h"

void init_alloc();

void *my_alloc(size_t size);

void my_free(void *ptr);

unsigned long long get_max();

unsigned long long get_all();

unsigned long long get_current();

#endif //SPOLAB15_MY_ALLOC_H
