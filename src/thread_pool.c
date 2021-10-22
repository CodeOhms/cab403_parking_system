#include "thread_pool.h"

/* format of a single request. */
typedef struct request_t
{
    void *(*func)(void *);
    void *args;
} request_t;

void *handle_requests_loop(void *args);

void thread_pool_init(thread_pool_t *self)
{
    pthread_mutex_init(&self->request_mutex, NULL);
    pthread_cond_init(&self->got_request, NULL);

    /* create the request-handling threads */
    for (size_t i = 0; i < NUM_HANDLER_THREADS; i++)
    {
        // CREATE ALL THE THREADS - Threads to call handle_requests_loop() function
        pthread_create(&self->p_threads[i], NULL, handle_requests_loop, self);
    }
}

void thread_pool_close(thread_pool_t *self)
{
    /* Close all threads: */
    for(size_t i = 0; i < NUM_HANDLER_THREADS; ++i)
    {
        pthread_join(self->p_threads[i], NULL);
    }
}

/*
 * function add_request(): add a request to the requests list
 * algorithm: creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * input:     request number, linked list mutex.
 * output:    none.
 */
void thread_pool_add_request(thread_pool_t *self, void *(*func)(void *), void *args)
{
    request_t *a_request; /* pointer to newly added request.     */

    /* create structure with new request */
    a_request = (request_t *)malloc(sizeof(request_t));
    if (!a_request)
    { /* malloc failed?? */
        fprintf(stderr, "add_request: out of memory\n");
        exit(1);
    }
    a_request->func = func;
    a_request->args = args;

    /* lock the mutex, to assure exclusive access to the list */
    if(&self->request_mutex != NULL)
    {
        pthread_mutex_lock(&self->request_mutex);
    }

    /* add new request to the end of the list, updating list */
    llist_append(&self->request_list, a_request, sizeof(request_t));

    /* increase total number of pending requests by one. */
    self->num_requests++;

    /* unlock mutex */
    if(&self->request_mutex != NULL)
    {
        pthread_mutex_unlock(&self->request_mutex);
    }

    /* signal the condition variable - there's a new request to handle */
    {
        pthread_cond_signal(&self->got_request);
    }
}

/*
 * function get_request(): gets the first pending request from the requests list
 *                         removing it from the list.
 * algorithm: creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * output:    pointer to the removed request, or NULL if none.
 * memory:    the returned request need to be freed by the caller.
 */
request_t *get_request(thread_pool_t *self)
{
    node_t *request_node = llist_pop(&self->request_list);
    request_t *a_request = NULL;
    if(request_node != NULL)
    { /* Empty list */
        a_request = (request_t *)request_node->data;
    }

    return a_request;
}

/*
 * function handle_request(): handle a single given request.
 * algorithm: prints a message stating that the given thread handled
 *            the given request.
 * input:     request pointer, id of calling thread.
 * output:    none.
 */
void handle_request(request_t *a_request)
{
    a_request->func(a_request->args);
}

/*
 * function handle_requests_loop(): infinite loop of requests handling
 * algorithm: forever, if there are requests to handle, take the first
 *            and handle it. Then wait on the given condition variable,
 *            and when it is signaled, re-do the loop.
 *            increases number of pending requests by one.
 * input:     id of thread, for printing purposes.
 * output:    none.
 */
void *handle_requests_loop(void *args)
{
    thread_pool_t *self = (thread_pool_t *)args;

    request_t *a_request;

    /* lock the mutex, to access the requests list exclusively. */
    pthread_mutex_lock(&self->request_mutex);

    /* while still running.... */
    int running = 1;
    while (running)
    {
        if (self->num_requests > 0)
        { /* a request is pending */
            a_request = get_request(self);
            if(a_request != NULL)
            { /* got a request - handle it and free it */
                /* unlock mutex - so other threads would be able to handle */
                /* other requests waiting in the queue paralelly.          */
                pthread_mutex_unlock(&self->request_mutex);

                //TO DO - UNLOCCK MUTEX, CALL FUNCTION TO HANDLE REQUEST AND RELOCK MUTEX
                // DYNAMIC MEMORY ALLOCATED _ HOW DO WE RECLAIM MEMORY
                handle_request(a_request);

                free(a_request);

                /* and lock the mutex again. */
                pthread_mutex_lock(&self->request_mutex);
            }
        }
        else
        {
            /* wait for a request to arrive. note the mutex will be */
            /* unlocked here, thus allowing other threads access to */
            /* requests list.                                       */

            pthread_cond_wait(&self->got_request, &self->request_mutex);
            /* and after we return from pthread_cond_wait, the mutex  */
            /* is locked again, so we don't need to lock it ourselves */
        }

        pthread_mutex_lock(&self->quit_mutex);
        if (self->quit) running = 0;
        pthread_mutex_unlock(&self->quit_mutex);
    }
    pthread_mutex_unlock(&self->request_mutex);
    return NULL;
}