/**
 * CS 2110 - Spring 2018 - Homework #10
 *
 * @author Patrick Tam + Sanjay Sood
 *
 * list.h: DO NOT MODIFY
 */

#ifndef LIST_H
#define LIST_H


/**
 * Performs operation on data pointer
 *
 * @param data void pointer containing data that operation is performed on
 */
typedef void (*list_op)(void* data);


/**
 * Copies a given data pointer
 * copy operation's memory allocations may fail,
 * will return error code if that happens
 *
 * @param data_to_copy void pointer containing data to be copied
 * @param copy_of_data pointer to pointer with copied data
 * @return non-zero error code if copy operation fails
 */
typedef int (*list_copy)(const void * data_to_copy, void** copy_of_data);


/**
 * Checks if two data pointers point to equal data
 *
 * @param data_a
 * @param data_b
 * @return non-zero integer if data is equal, 0 otherwise
 */
typedef int (*list_eq)(const void* data_a, const void* data_b);

// macro to hide silence "unused parameter" compiler warnings
#define UNUSED_PARAM(x) (void)(x)

#endif /* LIST_H */
