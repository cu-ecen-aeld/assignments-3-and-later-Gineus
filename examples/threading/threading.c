#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* td = (struct thread_data *) thread_param;

    usleep(td->wait_to_obtain_ms * 1000);

    int result = pthread_mutex_lock(td->mutex);

    if (result != 0)
    {
        ERROR_LOG("pthread_mutex_lock failed with error code %d", result);
        td->thread_complete_success = false;

        return thread_param;
    }

    usleep(td->wait_to_release_ms * 1000);
    td->thread_complete_success = true;

    result = pthread_mutex_unlock(td->mutex);

    if (result != 0)
    {
        ERROR_LOG("pthread_mutex_unlock failed with error code %d", result);
        td->thread_complete_success = false;

        return thread_param;
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *td = (struct thread_data *)malloc(sizeof(struct thread_data));

    if (td == NULL)
    {
        ERROR_LOG("Failed to allocate memory for thread_data");
        return false;
    }

    td->mutex = mutex;
    td->wait_to_obtain_ms = wait_to_obtain_ms;
    td->wait_to_release_ms = wait_to_release_ms;
    td->thread_complete_success = false;

    int result = pthread_create(thread, NULL, threadfunc, td);

    if (result != 0)
    {
        ERROR_LOG("pthread_create failed with error code %d", result);
        free(td);
        return false;
    }

    return true;
}

