#ifndef  THREAD_POOL
#define  THREAD_POOL

#define _GNU_SOURCE
#include <stdio.h>   /* standard I/O routines                     */
#include <pthread.h> /* pthread functions and data structures     */
#include <stdlib.h>  /* rand() and srand() functions              */
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "linked_list.h"

// #define NUM_HANDLER_THREADS 120
#define NUM_HANDLER_THREADS 5

typedef struct thread_pool_t
{
    pthread_t p_threads[NUM_HANDLER_THREADS];

    list_t request_list;
    pthread_mutex_t request_mutex;
    pthread_mutex_t quit_mutex;
    pthread_cond_t got_request;

    size_t num_requests;
    bool quit;
} thread_pool_t;

void thread_pool_init(thread_pool_t *self);

void thread_pool_close(thread_pool_t *self);

void thread_pool_add_request(thread_pool_t *self, void *(*func)(void *), void *args);

#endif //THREAD_POOL