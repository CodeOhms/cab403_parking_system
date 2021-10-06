#ifndef  TIMING_MANGER_H
#define  TIMING_MANGER_H

#include <unistd.h>

/**
 * @brief Initialise timing manager. Requires a function pointer to
 * register delayed functions to a thread pool, to run once delay has expired.
 * 
 * @param thread_pool_register function pointer to the register of a thread pool queue.
 * Takes a void pointer argument and returns void.
 * 
 * @returns Void.
 */
void timing_manager_init(void (*thread_pool_register)(void (*func)(void *)));

/**
 * @brief Use to register a function and its delay to the timing manager list.
 * 
 * @param
 * @param
 * 
 * @returns
 */
void timing_manager_register_function();

/**
 * @brief Responsible for calling registered functions after a specified delay.
 * 
 * @returns Void.
 */
void timing_manager_loop(void);

/**
 * @brief Call to stop timing manager.
 *
 * @returns Void.
 */
void stop_timing_manager(void);

#endif //TIMING_MANGER_H