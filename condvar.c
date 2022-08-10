#include "mu.h"
#include <linux/futex.h>

static void futex_lock(_Atomic int *futex)
{
    _Atomic int val = *futex;
    if (atomic_bool_cmpxchg(futex, val, 1))
        return;
    else {
        if (atomic_load_explicit(futex, memory_order_relaxed) == 2)
            goto futex;

        while (atomic_exchange_explicit(futex, 2, memory_order_acquire) != 0) {
            futex:
            SYSCALL3(__NR_futex, futex, FUTEX_WAIT_PRIVATE, 2);
        }
    }
}

static void futex_unlock(_Atomic int *futex)
{
    if (atomic_exchange_explicit(futex, 0, memory_order_release) == 2)
        SYSCALL3(__NR_futex, futex, FUTEX_WAKE_PRIVATE, 1);
}

int muthread_cond_wait(muthread_cond_t *condvar, muthread_mutex_t *mutex)
{
    muthread_mutex_unlock(mutex);
    futex_lock(&condvar->lock);
    do {
        futex_unlock(&condvar->lock);
        SYSCALL3(__NR_futex, &condvar->futex, FUTEX_WAIT_PRIVATE, 0);
        break;
    } while (1);
    muthread_mutex_lock(mutex);
    return 0;
}

int muthread_cond_signal(muthread_cond_t *condvar, muthread_mutex_t *mutex)
{
    muthread_mutex_unlock(mutex);
    futex_lock(&condvar->lock);
    SYSCALL3(__NR_futex, &condvar->futex, FUTEX_WAKE_PRIVATE, 1);
    futex_unlock(&condvar->lock);
    muthread_mutex_lock(mutex);
    return 0;
}

int muthread_cond_broadcast(muthread_cond_t *condvar, muthread_mutex_t *mutex)
{
    muthread_mutex_unlock(mutex);
    futex_lock(&condvar->lock);
    SYSCALL3(__NR_futex, &condvar->futex, FUTEX_WAKE_PRIVATE, INT32_MAX);
    futex_unlock(&condvar->lock);
    muthread_mutex_lock(mutex);
    return 0;
}

int muthread_cond_wait_pi(muthread_cond_t *condvar, muthread_mutex_t *mutex)
{
    futex_lock(&condvar->lock);
    muthread_mutex_unlock(mutex);
    do {
        futex_unlock(&condvar->lock);
        SYSCALL5(__NR_futex, &condvar->futex, FUTEX_WAIT_REQUEUE_PI, NULL, NULL, mutex);
        break;
    } while (1);
    return 0;
}

int muthread_cond_signal_pi(muthread_cond_t *condvar, muthread_mutex_t *mutex)
{
    muthread_mutex_unlock(mutex);
    futex_lock(&condvar->lock);
    SYSCALL5(__NR_futex, &condvar->futex, FUTEX_CMP_REQUEUE_PI, 1, 0, mutex);
    futex_unlock(&condvar->lock);
    return 0;
}

int muthread_cond_broadcast_pi(muthread_cond_t *condvar, muthread_mutex_t *mutex)
{
    muthread_mutex_unlock(mutex);
    futex_lock(&condvar->lock);
    SYSCALL5(__NR_futex, &condvar->futex, FUTEX_CMP_REQUEUE_PI, 1,  INT32_MAX, mutex);
    futex_unlock(&condvar->lock);
    return 0;
}