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

    /* Free memory of linked list management structure: */
    free(self);
}

node_t *llist_find(list_t *self, void *cmp_to)
{
    if(self->compare != NULL)
    {
        for(node_t* current = self->head; current != NULL; current = current->next)
        {
            if(current->data != NULL)
            { /* Need to check this as this impl allows a node with data == NULL. */
                if(self->compare(cmp_to, current->data) == 0)
                {
                    return current;
                }
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
    memcpy(new_node->data, data, data_size);

    return new_node;
}

node_t *llist_append_empty(list_t *self, size_t data_size)
{
    node_t *new_node = NULL;

    /* If an empty list, appending has the same affect as pushing: */
    if(self->head == NULL && self->tail == NULL)
    {
        new_node = llist_push_empty(self, data_size);
    }
    else
    {
        /* Allocate memory for the new node: */
        new_node = (node_t *)malloc(sizeof(node_t));

        /* Allocate memory for data: */
        new_node->data = malloc(data_size);

        /* Replace tail of the list with the new node: */
        node_t *old_tail = self->tail;
        self->tail = new_node;

        /* Set old tail to point to new node: */
        old_tail->next = new_node;

        /* Connect new tail (new node) previous pointer to old tail: */
        new_node->previous = old_tail;
    }

    /* New node is new tail, so it's next pointer goes nowhere: */
    new_node->next = NULL;

    return new_node;
}

node_t *llist_push(list_t *self, void *data, size_t data_size)
{
    node_t *new_node = llist_push_empty(self, data_size);

    /* Copy given data to new node: */
    memcpy(new_node->data, data, data_size);

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

    /* Old head previous pointer connect to new head: */
    if(old_head != NULL)
    { /* If it was an emtpy list, old head would be NULL. */
        old_head->previous = self->head;
    }

    /* Special cases to deal with tail of list: */
        /* Was an empty list: */
    if(old_head == NULL)
    {
        self->tail = self->head;
    }
        /* Was a one item list: */
    else if(self->head == self->tail)
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
    {
        previous_node->next = next_node;
    }
    else
    { /* If NULL, then at head of list. */
        self->head = next_node;
    }

    /* Next node previous pointer connect to previous node: */
    if(next_node != NULL)
    {
        next_node->previous = previous_node;
    }
    else
    { /* If NULL, at end of list. */
        self->tail = previous_node;
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

void llist_delete_dangling_node(node_t *dangling_node, void (*destructor)(void *node))
{
    if(destructor != NULL)
    {
        destructor(dangling_node);
    }

    free(dangling_node->data);
    free(dangling_node);
    dangling_node = NULL;
}