#include "linked_list.h"

void llist_init(list_t **self, int (*compare_func)(const void *data1, const void *data2),
    void (*destructor)(void *))
{
    /* Allocate memory for list management structure: */
    (*self) = (list_t *)malloc(sizeof(list_t));

    (*self)->head = NULL;
    (*self)->tail = NULL;
    (*self)->compare = compare_func;
    (*self)->destructor = destructor;
}

void llist_close(list_t *self)
{
    /* Call destructor for each list item: */
    node_t *current;
    while(self->head != NULL)
    {
        /* Make current node follow the head of the list as it shrinks: */
        current = self->head;
        self->head = current->next;

        if(self->destructor != NULL)
        {
            self->destructor(current);
        }

        /* Free memory of current node data, then the node itself: */
        free(current->data);
        free(current);
    }
}

node_t *llist_find(list_t *self, void *data)
{
    for(node_t* current = self->head; current != NULL; current = current->next)
    {
        if(self->compare(data, current->data) == 0)
        {
            return current;
        }
    }

    /* Nothing found: */
    return NULL;
}

node_t *llist_push(list_t *self, void *data, size_t data_size)
{
    node_t *new_node = NULL;

    if(self->head->next != NULL)
    { /* Not an empty list */
        new_node = llist_push_empty(self, data_size);

        /* Copy given data to new node: */
        memcpy(new_node, data, data_size);
    }

    return new_node;    
}

node_t *llist_push_empty(list_t *self, size_t data_size)
{
    node_t *new_node = NULL;

    // if(self->head->next != NULL)
    // { /* Not an empty list */
    /* Create new node and allocate memory: */
    new_node = (node_t *)malloc(sizeof(node_t));

    /* Allocate memory for data: */
    void *data = malloc(data_size);

    new_node->data = data;

    /* Replace head of the list with new node: */
    node_t *old_head = self->head;
    self->head = new_node;
    self->head->previous = NULL;

    /* New head point to old head: */
    self->head->next = old_head;

    if(old_head != NULL)
    { /* The list was an empty list: */
        /* Old head previous pointer connect to new head: */
        old_head->previous = self->head;
    }

    return new_node;
}

void llist_delete_node(node_t *node)
{
    if(node->next != NULL)
    { /* If NULL, then at end of list. */
    /* Next node previous pointer connect to previous node: */
        node->next->previous = node->previous;
    }

    /* Previous node point to next node: */
    if(node->previous != NULL)
    { /* If NULL, at head of list. */
        node->previous->next = node->next;
    }

    /* Delete node: */
    free(node->data);
    free(node);
    node = NULL;
}