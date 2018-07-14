/**
 * @author Harrison Wilco
 *
 * array list implementation
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "array_list.h"

int array_list_resize(array_list_t* arr, int n);
int null_out(array_list_t* arr, int start, int end);

/**
 * Creates an empty array list
 * - array list's size should be 0, cuz it's empty
 * - allocates a backing array of size INITIAL_CAPACITY
 * - array list's capacity should be the size of the backing array
 * - the backing array should be empty (all entries should be NULL)
 * - if any memory allocation operations fail, return NULL and
 *   clean up any dangling pointers
 *
 * @return the newly created array list
 */
array_list_t* create_array_list(void)
{
    array_list_t* arr = (array_list_t*) malloc(sizeof(array_list_t));
    if (arr == NULL) {
        return NULL;
    }
    arr->size = 0;
    arr->capacity = INITIAL_CAPACITY;
    arr->entries = (void**) calloc(INITIAL_CAPACITY, sizeof(void*));
    if (arr->entries == NULL) {
        free(arr);
        return NULL;
    }
    return arr;
}


/**
 * Destroys an array list
 * - all memory allocated to the array list should be freed
 * - all memory allocated to each data entry in the array should be freed
 * - return non-zero if either parameter is NULL
 *
 * @param arr array list that you're destroying
 * @param free_func function that frees a data pointer
 * @return 0 if operation succeeded, non-zero otherwise
 */
int destroy_array_list(array_list_t* arr, list_op free_func)
{
    if(arr == NULL || free_func == NULL) {
        return 1;
    }
    for (int i = 0; i < arr->size; i++) {
        free_func(arr->entries[i]);
    }
    free(arr->entries);
    free(arr);
    return 0;
}

/**
 * Copies the array list (shallow copy)
 * - creates copy of the array_list_t struct
 * - creates copy of the array list's backing array
 * - DOES NOT create a copy of data pointed at by each data pointer
 * - if list_to_copy is NULL, return NULL
 * - if any memory allocation operations fail, return NULL and
 *   clean up any dangling pointers
 * - list_to_copy should not be changed by this function
 *
 * @param list_to_copy array list you're copying
 * @return the shallow copy of the given array list
 */
array_list_t* shallow_copy_array_list(array_list_t* list_to_copy)
{
    if (list_to_copy == NULL) {
        return NULL;
    }
    array_list_t* arr = (array_list_t*) malloc(sizeof(array_list_t));
    if (arr == NULL) {
        return NULL;
    }
    arr->size = list_to_copy->size;
    arr->capacity = list_to_copy->capacity;
    arr->entries = (void**) calloc(arr->capacity, sizeof(void*));
    if (arr->entries == NULL) {
        free(arr);
        return NULL;
    }
    memcpy(arr->entries, list_to_copy->entries, list_to_copy->size * sizeof(list_to_copy->entries[0]));
    /*
    for (int i = 0; i < arr->size; i++) {
        arr->entries[i] = list_to_copy->entries[i];
    }
    */
    return arr;
}

/**
 * Adds the given data at the given index
 * - if the index is negative, wrap around (ex: -1 becomes size - 1)
 * - if size + index < 0, keep wrapping around until a positive in bounds index is reached
 *   (ex: -5 with size 4 becomes index 3)
 * - adding in the middle of the array list (index < size)
 *     - increment size
 *     - if necessary, resize the backing array as described below
 *     - shift all elements with indices [index, size - 1] down by one
 *     - the indices of the shifted elements should now be [index + 1, size])
 *     - put data into index in the array list
 * - adding at the end of the array list (index == size)
 *     - increment size
 *     - if necessary, resize the backing array as described below
 *     - put data into the index in the array list
 * - adding after the end of the array list (index > size)
 *     - index now represents the last element in the array list, so size = index + 1
 *     - if necessary, resize the backing array as described below
 *     - put data into the index in the array list
 *     - Note: this operation will result in 1 or more empty slots from [old size, index - 1]
 * - if any of these operations would increase size to size >= capacity, you must
 *   increase the size of the backing array by GROWTH_FACTOR until size < capacity.
 *   Only then can you insert into the backing array
 * - if any memory allocation operations fail, do not change the array list and return non-zero
 *     - the array list should be in the same state it was in before add_to_array_list was called
 * - if arr is NULL, return non-zero
 * - data can be NULL
 *
 * @param arr array list you're adding to
 * @param index you're adding data to
 * @param data data you're putting into the array list
 * @return 0 if add operation succeeded, non-zero otherwise
 */
int add_to_array_list(array_list_t* arr, int index, void* data)
{
    if (arr == NULL) {
        return 1;
    }
    index = (index < 0)? index % arr->size + arr->size : index;
    int newCapacity = arr->capacity;
    int capacityBackup = arr->capacity;
    int sizeBackup = arr->size;
    // inserting into middle of array
    if (index <= arr->size) {
        arr->size++;
    } else {
        arr->size = index + 1;
    }
    if(arr->size >= arr->capacity) {
        while(arr->size >= newCapacity) {
            newCapacity *= GROWTH_FACTOR;
        }
        int resized = array_list_resize(arr, newCapacity);
        if (resized != 0) {
            arr->size = sizeBackup;
            return 1;
        }
        null_out(arr, capacityBackup, arr->capacity - 1);
        /*
        for (int i = arr->size; i < arr->capacity; i++) {
            arr->entries[i] = NULL;
        }
        */
    }
    if (index < arr->size) {
        memmove(&arr->entries[index+1],&arr->entries[index], sizeof(void*) * (arr->size - 1 - index));
        /*for (int i = arr->size; i > index; i--) {
            arr->entries[i] = arr->entries[i-1];
        }*/
    }
    arr->entries[index] = data;
    return 0;
}


/**
 * Removes the data at the given index and returns it to the user
 * - if the index is negative, wrap around (ex: -1 becomes size - 1)
 * - if size + index < 0, keep wrapping around until a positive in bounds index is reached
 *   (ex: -5 with size 4 becomes index 3)
 * - removing from the middle of the array list (index < size - 1)
 *     - save the data pointer at index
 *     - shift all elements with indices [index + 1, size - 1] up by one
 *     - the indices of the shifted elements should now be [index, size - 2])
 *     - decrement the size and resize backing array if necessary
 *     - return the saved data pointer
 * - removing from the end of the array list (index == size - 1)
 *     - save the data pointer at index
 *     - decrement the size and resize backing array if necessary
 *     - return the saved data pointer
 * - removing from after the end of the array list (index > size - 1)
 *     - there's nothing to remove, so fail by returning NULL
 * - if any of these operations would decrease size to size < capacity / GROWTH_FACTOR ^ 2, you must
 *   shrink the backing array by GROWTH_FACTOR so that capacity = previous capacity / GROWTH_FACTOR
 * - if arr or data_out are NULL, return non-zero
 *
 * @param arr array list you're removing from
 * @param index you're removing from array list
 * @param data_out pointer where you store data pointer removed from array list
 * @return 0 if remove operation succeeded, non-zero otherwise
 */
int remove_from_array_list(array_list_t* arr, int index, void** data_out)
{
    if (data_out == NULL) {
        return 1;
    }
    if (arr == NULL) {
        *data_out = NULL;
        return 1;
    }
    index = (index < 0)? index % arr->size + arr->size : index;
    if (index >= arr->size) {
        *data_out = NULL;
        return 1;
    }
    *data_out = arr->entries[index];
    if (index < arr->size - 1) {
        memmove(&arr->entries[index], &arr->entries[index+1], sizeof(void*) * (arr->size - index - 1));
    }
    null_out(arr, arr->size - 1, arr->size - 1);
    /*for (int i = index; i < arr->size - 1; i++) {
        arr->entries[i] = arr->entries[i+1];
    }*/
    arr->size--;
    if (arr->size < arr->capacity / (GROWTH_FACTOR * GROWTH_FACTOR)) {
        int n = arr->capacity / GROWTH_FACTOR;
        array_list_resize(arr, n);
        /*arr->capacity = arr->capacity / GROWTH_FACTOR;
        arr->entries = realloc(arr->entries, arr->capacity);*/
        //null_out(arr, arr->size, arr->capacity - 1);
    }
    return 0;
}


/** TODO: NOTHING!!!! DO NOT IMPLEMENT
 * Checks if array list contains data
 * - checks if each data pointer from indices [0, size - 1]
 *   equal the passed in data using eq_func
 * - terminate search on first match
 * - if data is found, return it through return_found
 * - if any parameters besides data are NULL, return false
 *
 * @param arr array list you're searching
 * @param data the data you're looking for
 * @param eq_func function used to check if two data pointers are equal  (returns non-zero if equal)
 * @param return_found pointer through which you return found data if found
 * @return true if array list contains data
 */
bool array_list_contains(array_list_t* arr, void* data, list_eq eq_func, void** return_found)
{
    if (return_found == NULL) {
        return false;
    }
    if (arr == NULL || data == NULL || eq_func == NULL) {
        *return_found = NULL;
        return false;
    }
    for (int i = 0; i < arr->size; i++) {
        if (eq_func(data, arr->entries[i]) != 0) {
            *return_found = arr->entries[i];
            return true;
        }
    }
    return false;
}


/**
 * Trims backing array's size so it's the same as the size of the array
 * - if any memory allocation operations fail, do not change the array list and return false
 * - the array list should be in the same state it was in before add_at_index was called
 * - if arr is NULL, return non-zero
 *
 * @param arr array list who's backing array you're trimming
 * @return 0 if remove operation succeeded, non-zero otherwise
 */
int trim_to_size(array_list_t* arr)
{
    if (arr==NULL) {
        return 1;
    }
    return array_list_resize(arr, arr->size);
}


int array_list_resize(array_list_t* arr, int n) {
    if (arr == NULL || n < 0) {
        return 1;
    }
    void** dummy = (void**) realloc(arr->entries, sizeof(void*) * n);
    if (dummy == NULL) {
        return 1;
    }
    arr->entries = dummy;
    arr->capacity = n;
    return 0;
}


/**
 * Places NULL in arr->entries from start to end, inclusive of end
 */
int null_out(array_list_t* arr, int start, int end) {
    memset(&arr->entries[start], 0, sizeof(void*) * (end - start + 1));
    return 0;
}