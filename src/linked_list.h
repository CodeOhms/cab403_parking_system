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

/**
 * @brief Put given data into a new node at the head of the linked list.
 * NOTE: requires the given data to already have memory allocated.
 */
node_t *llist_push(list_t *self, void *data, size_t data_size);

node_t *llist_push_empty(list_t *self, size_t data_size);

void llist_delete_node(node_t *node);

#endif //LINKED_LIST_H