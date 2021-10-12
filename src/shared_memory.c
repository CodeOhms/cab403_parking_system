#include "shared_memory.h"

bool shared_mem_attach(shared_mem_t* shm)
{
    // Get a file descriptor connected to shared memory object and save in 
    // shm->fd. If the operation fails, ensure that shm->data is 
    // NULL and return false.
    if((shm->fd = shm_open(shm_name, O_RDWR, 0)) == -1)
    {
        shm->data = NULL;
        return false;
    }

    // Otherwise, attempt to map the shared memory via mmap, and save the address
    // in shm->data. If mapping fails, return false.
    if((shm->data = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0)) == MAP_FAILED)
    {
        return false;
    }

    // Modify the remaining stub only if necessary.
    return true;
}