/**
 * CS 2110 - Spring 2018 - Homework #10
 *
 * @author Patrick Tam + Sanjay Sood
 *
 * array_list.h: DO NOT MODIFY
 */

#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include <stdbool.h>
#include "list.h"

// initial capacity of backing array
#define INITIAL_CAPACITY 10

// backing array growth factor
#define GROWTH_FACTOR 2

typedef struct _array_list_t {

    // size of array (greatest index + 1)
    int size;

    // capacity of backing array (largest number of elements it can hold)
    int capacity;

    // the backing array
    void** entries;

} array_list_t;

/* Creating */
array_list_t* create_array_list(void);
array_list_t* shallow_copy_array_list(array_list_t*);

/* Destroying */
void free_array_list(array_list_t *arr, list_op free_func);

/* Adding */
int add_to_array_list(array_list_t* arr, int index, void* data);

/* Querying */
bool array_list_contains(array_list_t* arr, void* data, list_eq eq_func, void** return_found);

/* Removing */
int remove_from_array_list(array_list_t* arr, int index, void** data_out);

/* Mutators */
int trim_to_size(array_list_t*);

#endif /* ARRAY_LIST_H */