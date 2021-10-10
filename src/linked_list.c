#include "linked_list.h"

void llist_init(list_t *self, int (*compare_func)(const void *data1, const void *data2))
{
    /* Allocate memory: */
    self = (list_t *)malloc(sizeof(list_t));

    self->head = NULL;
    self->tail = NULL;
    self->compare = compare_func;
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

node_t *llist_push(node_t *head, void *data, size_t data_size)
{
    node_t *new_node = NULL;

    if(head->next != NULL)
    { /* Not an empty list */
        new_node = llist_push_empty(head, data_size);

        /* Copy given data to new node: */
        memcpy(new_node, data, data_size);
    }

    return new_node;    
}

node_t *llist_push_empty(node_t *head, size_t data_size)
{
    node_t *new_node = NULL;

    if(head->next != NULL)
    { /* Not an empty list */
        /* Create new node and allocate memory: */
        new_node = (node_t *)malloc(sizeof(node_t));

        /* Allocate memory for data: */
        void *data = malloc(data_size);

        /* 1st node previous pointer connect to new node: */
        head->next->previous = new_node;

        /* New node point to 1st node: */
        new_node->next = head->next;

        /* New node previous pointer connect to head: */
        new_node->previous = head;

        /* Set head to be (point) to new node: */
        head->next = new_node;
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