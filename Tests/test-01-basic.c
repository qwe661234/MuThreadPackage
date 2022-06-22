#include <stdlib.h>
#include <string.h>

#include "mu.h"

/* Test normal mutex */
void *thread_func_normal(void *arg)
{
    muthread_t self = muthread_self();
    muthread_mutex_t *mutex = (muthread_mutex_t *) arg;
    int locked = 0;
    if (!muthread_mutex_trylock(mutex)) {
        muprint("[thread 0x%llx] Trying to acquire the mutex succeeded\n",
                self);
        locked = 1;
    } else
        muprint("[thread 0x%llx] Trying to acquire the mutex failed\n", self);

    if (!locked)
        muthread_mutex_lock(mutex);
    muprint("[thread 0x%llx] Starting normal mutex test\n", self);
    musleep(1);
    muprint("[thread 0x%llx] Finishing normal mutex test\n", self);
    muthread_mutex_unlock(mutex);
    return 0;
}

/* Test errorcheck mutex */
void *thread_func_errorcheck1(void *arg)
{
    muthread_t self = muthread_self();
    muthread_mutex_t *mutex = (muthread_mutex_t *) arg;

    muthread_mutex_lock(mutex);
    muprint("[thread 0x%llx] Starting errorcheck mutex test\n", self);
    if (muthread_mutex_lock(mutex) == -EDEADLK)
        muprint("[thread 0x%llx] Trying to lock again would deadlock\n", self);
    musleep(2);
    muprint("[thread 0x%llx] Finishing errorcheck mutex test\n", self);
    muthread_mutex_unlock(mutex);
    return 0;
}

void *thread_func_errorcheck2(void *arg)
{
    muthread_t self = muthread_self();
    muthread_mutex_t *mutex = (muthread_mutex_t *) arg;
    musleep(1);
    if (muthread_mutex_unlock(mutex) == -EPERM)
        muprint("[thread 0x%llx] Trying to unlock a mutex we don't own\n",
                self);
    muthread_mutex_lock(mutex);
    muprint("[thread 0x%llx] Starting errorcheck mutex test\n", self);
    musleep(1);
    muprint("[thread 0x%llx] Finishing errorcheck mutex test\n", self);
    muthread_mutex_unlock(mutex);
    if (muthread_mutex_unlock(mutex) == -EPERM)
        muprint("[thread 0x%llx] Trying to unlock unlocked mutex\n", self);
    return 0;
}

/* Test recursive mutex */
void *thread_func_recursive(void *arg)
{
    muthread_t self = muthread_self();
    muthread_mutex_t *mutex = (muthread_mutex_t *) arg;
    int locked = 0;
    if (!muthread_mutex_trylock(mutex)) {
        muprint("[thread 0x%llx] Trying to acquire the mutex succeeded\n",
                self);
        locked = 1;
    } else
        muprint("[thread 0x%llx] Trying to acquire the mutex failed\n", self);

    if (!locked)
        muthread_mutex_lock(mutex);
    muthread_mutex_lock(mutex);
    muthread_mutex_lock(mutex);
    muprint("[thread 0x%llx] Starting recursive mutex test\n", self);
    musleep(1);
    muprint("[thread 0x%llx] Finishing recursive mutex test\n", self);
    muthread_mutex_unlock(mutex);
    muthread_mutex_unlock(mutex);
    muthread_mutex_unlock(mutex);
    return 0;
}

int main(int argc, char **argv)
{
    muthread_t thread[5];
    muthread_attr_t attr;
    int st;

    /* Initialize the mutexes */
    muthread_mutexattr_t mattr;
    muthread_mutex_t mutex_normal;
    muthread_mutex_t mutex_errorcheck;
    muthread_mutex_t mutex_recursive;

    muthread_mutexattr_init(&mattr);
    muthread_mutex_init(&mutex_normal, 0);
    muthread_mutexattr_settype(&mattr, TBTHREAD_MUTEX_ERRORCHECK);
    muthread_mutex_init(&mutex_errorcheck, &mattr);
    muthread_mutexattr_settype(&mattr, TBTHREAD_MUTEX_RECURSIVE);
    muthread_mutex_init(&mutex_recursive, &mattr);

    /* Spawn the threads to test the normal mutex */
    muprint("[thread main] Testing normal mutex\n");
    muthread_attr_init(&attr);
    for (int i = 0; i < 5; ++i) {
        st = muthread_create(&thread[i], &attr, thread_func_normal,
                             &mutex_normal);
        if (st != 0) {
            muprint("Failed to spawn thread %d: %s\n", i, strerror(-st));
            return 1;
        }
    }

    muprint("[thread main] Threads spawned successfully\n");
    muprint("[thread main] Sleeping 7 seconds\n");
    musleep(7);

    /* Spawn threads to thest the errorcheck mutex */
    void *(*err_check_func[2])(void *) = {thread_func_errorcheck1,
                                          thread_func_errorcheck2};
    muprint("---\n");
    muprint("[thread main] Testing errorcheck mutex\n");
    for (int i = 0; i < 2; ++i) {
        st = muthread_create(&thread[i], &attr, err_check_func[i],
                             &mutex_errorcheck);
        if (st != 0) {
            muprint("Failed to spawn thread %d: %s\n", i, strerror(-st));
            return 1;
        }
    }

    muprint("[thread main] Threads spawned successfully\n");
    muprint("[thread main] Sleeping 5 seconds\n");
    musleep(5);

    /* Spawn the threads to test the recursive mutex */
    muprint("---\n");
    muprint("[thread main] Testing recursive mutex\n");
    muthread_attr_init(&attr);
    for (int i = 0; i < 5; ++i) {
        st = muthread_create(&thread[i], &attr, thread_func_recursive,
                             &mutex_recursive);
        if (st != 0) {
            muprint("Failed to spawn thread %d: %s\n", i, strerror(-st));
            return 1;
        }
    }

    muprint("[thread main] Threads spawned successfully\n");
    muprint("[thread main] Sleeping 7 seconds\n");
    musleep(7);

    return 0;
}