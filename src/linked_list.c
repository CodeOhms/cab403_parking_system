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
        if(current->data != NULL)
        { /* Need to check this as this impl allows a node with data == NULL. */
            if(self->compare(data, current->data) == 0)
            {
                return current;
            }
        }
    }

    /* Nothing found: */
    return NULL;
}

node_t *llist_append(list_t *self, void *data, size_t data_size)
{
    node_t *new_node = llist_append_empty(self, data_size);

    /* Copy given data to new node: */
    memcpy(new_node, data, data_size);

    return new_node;
}

node_t *llist_append_empty(list_t *self, size_t data_size)
{
    node_t *new_node = NULL;

    /* Create new node and allocate memory: */
    new_node = (node_t *)malloc(sizeof(node_t));

    /* Allocate memory for data: */
    void *data = malloc(data_size);

    new_node->data = data;
    new_node->next = NULL;

    /* If an empty list, appending has the same affect as pushing: */
    if(self->head == NULL && self->tail == NULL)
    {
        llist_push_empty(self, data_size);
    }
    else
    {
        /* Replace tail of the list with the new node: */
        node_t *old_tail = self->tail;
        self->tail = new_node;

        /* Set old tail to point to new node: */
        old_tail->next = new_node;

        /* Connect new tail (new node) previous pointer to old tail: */
        new_node->previous = old_tail;
    }

    return new_node;
}

node_t *llist_push(list_t *self, void *data, size_t data_size)
{
    node_t *new_node = llist_push_empty(self, data_size);

    /* Copy given data to new node: */
    memcpy(new_node, data, data_size);

    return new_node;
}

node_t *llist_push_empty(list_t *self, size_t data_size)
{
    node_t *new_node = NULL;

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

    /* Special cases to deal with tail of list: */
        /* Empty list: */
    if(self->head == NULL && self->tail == NULL)
    {
        self->tail = self->head;
    }
        /* One item list: */
    if(self->head == self->tail)
    {
        /* Old head becomes the tail: */
        self->tail = old_head;
    }

    return new_node;
}

node_t *llist_pop(list_t *self)
{
    node_t *head = NULL;

    /* Not an empty list: */
    if(self->head != NULL)
    {
        head = self->head;

        /* Single item list, head and tail become NULL: */
        if(self->head == self->tail)
        {
            self->head = NULL;
            self->tail = NULL;
        }
        /* Two item list, head and tail become equal: */
        else if(self->head->next == self->tail)
        {
            self->head = self->tail;
        }
        /* At least three items, normal op.: */
        else
        {
            node_t *old_head = self->head;
            
            /* Second item becomes new head: */
            self->head = old_head->next;
            self->head->previous = NULL;
        }
    }

    return head;
}

void llist_delete_node(list_t *self, node_t *node)
{
    node_t *previous_node = node->previous;
    node_t *next_node = node->next;

    /* Prior node point to next node: */
    if(previous_node != NULL)
    { /* If NULL, then at end of list. */
        previous_node->next = next_node;
    }

    /* Next node previous pointer connect to previous node: */
    if(next_node != NULL)
    { /* If NULL, at head of list. */
        next_node->previous = previous_node;
    }

    if(self->destructor != NULL)
    {
        self->destructor(node);
    }

    /* Delete node: */
    free(node->data);
    free(node);
    node = NULL;
}