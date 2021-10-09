#include <stdbool.h>
#include "include/timing_manager.h"

const unsigned int time_quantum_ms = 1; /* Smallest unit of time delay, in milliseconds. */
bool _quit = false;
void (*_thread_pool_register)(void (*func)(void *), void *);

struct task_node
{
    void (*func)(void *);
    void *args;
    unsigned int delay_ms;
    struct task_node *next;
};

struct task_node *first_task = NULL; /* Head of linked list of delayed functions. */
struct task_node *last_task = NULL; /* Tail of linked list of delayed functions. */

void timing_manager_init(void (*thread_pool_register)(void (*func)(void *), void *))
{

}

/**
 * @brief Remove a registered function from the list of delayed tasks.
 * 
 * PRE: 
 *  
 */
void remove_list_node(struct task_node *node)
{
    
}

void timing_manager_loop(void)
{
    _quit = false;

    /* Run in an infinite loop, unless the signal to quit has been given: */
    while(!_quit)
    {
        /* Run through list of functions and pass to thread pool: */
        struct task_node *current_task = first_task;
        while(current_task != NULL)
        { /* Not at end of list. */
            /* Decrement delay: */
            --current_task->delay_ms;

            /* Check for timing trigger, if so run function: */
            if(current_task->delay_ms == 0)
            {
                /* Register function into the thread pool: */
                _thread_pool_register(current_task->func, current_task->args);

                /* Remove from list of functions to call: */
                remove_list_node(current_task);
            }

            current_task = current_task->next;
        }
    }
}

void stop_timing_manager(void)
{
    _quit = true;
}