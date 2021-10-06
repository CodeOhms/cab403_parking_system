#include <stdbool.h>
#include "include/timing_manager.h"

bool _quit = false;
void (*_thread_pool_register)(void (*func)(void *));

void timing_manager_loop(void)
{
    _quit = false;

    /* Run through list of functions and pass to thread pool: */
    for()
    {
        /* Check for timing trigger, if so run function: */
        if()
        {
            /* Register function into the thread pool and remove from list of functions to call: */
            _thread_pool_register(func(data));
        }
    }
}

void stop_timing_manager(void)
{
    _quit = true;
}