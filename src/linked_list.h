#ifndef  LINKED_LIST_H
#define  LINKED_LIST_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct node_t node_t;

typedef struct node_t
{
    void *data;
    node_t *previous;
    node_t *next;
} node_t;

typedef struct list_t
{
    node_t *head;
    node_t *tail;
    int (*compare)(const void *data1, const void *data2);
    void (*destructor)(void *node);
} list_t;

void llist_init(list_t **self, int (*compare_func)(const void *data1, const void *data2),
    void (*destructor)(void *));

void llist_close(list_t *self);

node_t *llist_find(list_t *self, void *data);

node_t *llist_append(list_t *self, void *data, size_t data_size);

node_t *llist_append_empty(list_t *self, size_t data_size);

/**
 * @brief Copy given data into a new node at the head of the linked list.
 */
node_t *llist_push(list_t *self, void *data, size_t data_size);

/**
 * @brief Create a new node, with data == NULL, at the head of the linked list.
 */
node_t *llist_push_empty(list_t *self, size_t data_size);

/**
 * @brief Pop the head off the linked list.
 * 
 * @returns Returns the first element of the linked list.
 * Also, disconnects this first element. Returns NULL if an empty list.
 */
node_t *llist_pop(list_t *self);

void llist_delete_node(list_t *self, node_t *node);

/**
 * @brief Use this function to delete nodes that are not connected to a linked list.
 * For example, if a node is popped of a list and deletion needs to be delayed.
 * Sets the given node to NULL.
 */
void llist_delete_dangling_node(node_t *dangling_node, void (*destructor)(void *node));

#endif //LINKED_LIST_H