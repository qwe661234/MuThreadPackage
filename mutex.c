#include "mu.h"

#include <linux/futex.h>
#include <sched.h>
#include <stdatomic.h>

/* Normal mutex */
static int lock_normal(muthread_mutex_t *mutex)
{  
    if (atomic_bool_cmpxchg(&mutex->futex, 0, 1))
        return 0;
    else {
        if (atomic_load_explicit(&mutex->futex, memory_order_relaxed) == 2)
            goto futex;

        while (atomic_exchange_explicit(&mutex->futex, 2, memory_order_acquire) != 0) {
            futex:
            SYSCALL3(__NR_futex, &mutex->futex, FUTEX_WAIT_PRIVATE, 2);
        }
    }
    return 0;
}

static int trylock_normal(muthread_mutex_t *mutex)
{
    if (atomic_bool_cmpxchg(&mutex->futex, 0, 1))
        return 0;
    return -EBUSY;
}

static int unlock_normal(muthread_mutex_t *mutex)
{
    if (atomic_exchange_explicit(&mutex->futex, 0, memory_order_release) == 2)
        SYSCALL3(__NR_futex, &mutex->futex, FUTEX_WAKE_PRIVATE, 1);
    return 0;
}

/* Init attributes */
int muthread_mutexattr_init(muthread_mutexattr_t *attr)
{
    attr->type = TBTHREAD_MUTEX_DEFAULT;
    attr->prioceiling = 0;
    return 0;
}

/* Set attributes */
int muthread_mutexattr_settype(muthread_mutexattr_t *attr, int type)
{
    if (type < TBTHREAD_MUTEX_NORMAL || type > TBTHREAD_MUTEX_ERRORCHECK)
        return -EINVAL;
    attr->type = type;
    return 0;
}

int muthread_mutexattr_setprotocol(muthread_mutexattr_t *attr, int protocol)
{
    if (protocol != TBTHREAD_PRIO_NONE && protocol != TBTHREAD_PRIO_INHERIT
      && protocol != TBTHREAD_PRIO_PROTECT)
        return -EINVAL;
    attr->type = (attr->type % 3) + protocol;
    return 0;
}

int muthread_mutexattr_setprioceiling(muthread_mutexattr_t *attr, int prioceiling)
{
    uint16_t prio_max = 99, prio_min = 1;
    if (prioceiling > prio_max || prioceiling < prio_min)
        return -EINVAL;
    attr->prioceiling = prioceiling;
    return 0;
}

/* Initialize the mutex */
int muthread_mutex_init(muthread_mutex_t *mutex,
                        const muthread_mutexattr_t *attr)
{
    uint16_t type = TBTHREAD_MUTEX_DEFAULT;
    uint16_t prioceiling = 0;
    if (attr) {
        type = attr->type;
        prioceiling = attr->prioceiling;
    } 
    mutex->futex = 0;
    mutex->type = type;
    mutex->prioceiling = prioceiling;
    mutex->owner = 0;
    mutex->counter = 0;
    return 0;
}

/* Lock the mutex */
int muthread_mutex_lock(muthread_mutex_t *mutex)
{
    muthread_t self = NULL;
    uint16_t type = mutex->type;
    switch(mutex->type){
    case TBTHREAD_MUTEX_NORMAL:
        if (atomic_bool_cmpxchg(&mutex->futex, 0, 1))
            return 0;
        else {
            if (atomic_load_explicit(&mutex->futex, memory_order_relaxed) == 2)
                goto futex;

            while (atomic_exchange_explicit(&mutex->futex, 2, memory_order_acquire) != 0) {
                futex:
                SYSCALL3(__NR_futex, &mutex->futex, FUTEX_WAIT_PRIVATE, 2);
            }
        }
        return 0;
    case TBTHREAD_MUTEX_ERRORCHECK:
        self = muthread_self();
        if (mutex->owner == self)
            return -EDEADLK;

        lock_normal(mutex);
        mutex->owner = self;
        return 0;
    case TBTHREAD_MUTEX_RECURSIVE:
        self = muthread_self();
        if (mutex->owner != self) {
            lock_normal(mutex);
            mutex->owner = self;
        }
        /* Check counter overflow */
        if (mutex->counter == (uint64_t) -1)
            return -EAGAIN;

        ++mutex->counter;
        return 0;
    case TBTHREAD_MUTEX_PI_NORMAL:
    case TBTHREAD_MUTEX_PI_ERRORCHECK:
    case TBTHREAD_MUTEX_PI_RECURSIVE:
        self = muthread_self();
        if (mutex->owner == self && type == TBTHREAD_MUTEX_PI_ERRORCHECK)
            return -EDEADLK;
        if (type == TBTHREAD_MUTEX_PI_RECURSIVE && mutex->counter == (uint64_t) -1)
            return -EAGAIN;

        SYSCALL2(__NR_futex, mutex, FUTEX_LOCK_PI);
        mutex->owner = self;
        
        if (type == TBTHREAD_MUTEX_PI_RECURSIVE)
            ++mutex->counter;
        return 0;
    case TBTHREAD_MUTEX_PP_NORMAL:
    case TBTHREAD_MUTEX_PP_ERRORCHECK:
    case TBTHREAD_MUTEX_PP_RECURSIVE:
        self = muthread_self();
        if (mutex->owner == self && type == TBTHREAD_MUTEX_PP_ERRORCHECK)
            return -EDEADLK;
        if (type == TBTHREAD_MUTEX_PP_RECURSIVE && mutex->counter == (uint64_t) -1)
            return -EAGAIN;

        lock_normal(mutex);
        mutex->owner = self;
        if (type == TBTHREAD_MUTEX_PP_RECURSIVE)
            ++mutex->counter;
        int ceiling = mutex->prioceiling;
        if (change_muthread_priority(self, ceiling) < 0)
            muprint("fail to change priority\n");
        return 0;
    }
    return 0;
}
/* Try locking the mutex */
int muthread_mutex_trylock(muthread_mutex_t *mutex)
{
    muthread_t self = NULL;
    uint16_t type = mutex->type;
    int ret = 0;
    switch(mutex->type){
    case TBTHREAD_MUTEX_NORMAL:
        if (atomic_bool_cmpxchg(&mutex->futex, 0, 1))
            return 0;
        return -EBUSY;
    case TBTHREAD_MUTEX_ERRORCHECK:
        self = muthread_self();
        if (mutex->owner == self)
            return -EDEADLK;

        lock_normal(mutex);
        mutex->owner = self;
        return 0;
    case TBTHREAD_MUTEX_RECURSIVE:
        self = muthread_self();
        if (mutex->owner != self && trylock_normal(mutex))
            return -EBUSY;

        if (mutex->owner != self) {
            mutex->owner = self;
        }
        /* Check counter overflow */
        if (mutex->counter == (uint64_t) -1)
            return -EAGAIN;

        ++mutex->counter;
        return 0;
    case TBTHREAD_MUTEX_PI_NORMAL:
    case TBTHREAD_MUTEX_PI_ERRORCHECK:
    case TBTHREAD_MUTEX_PI_RECURSIVE:
        self = muthread_self();
        if (mutex->owner == self && type == TBTHREAD_MUTEX_PI_ERRORCHECK)
            return -EDEADLK;
        if (type == TBTHREAD_MUTEX_PI_RECURSIVE && mutex->counter == (uint64_t) -1)
            return -EAGAIN;

        ret = SYSCALL2(__NR_futex, mutex, FUTEX_TRYLOCK_PI);
        if (ret == 0) {
            mutex->owner = self;
            if (type == TBTHREAD_MUTEX_PI_RECURSIVE)
                ++mutex->counter;
        }
        return ret;
    case TBTHREAD_MUTEX_PP_NORMAL:
    case TBTHREAD_MUTEX_PP_ERRORCHECK:
    case TBTHREAD_MUTEX_PP_RECURSIVE:
        self = muthread_self();
        if (mutex->owner == self && type == TBTHREAD_MUTEX_PP_ERRORCHECK) {
            return -EDEADLK;
        }
        if (type == TBTHREAD_MUTEX_PP_RECURSIVE && mutex->counter == (uint64_t) -1)
            return -EAGAIN;

        ret = trylock_normal(mutex);
        if (ret == 0) {
            mutex->owner = self;
            if (type == TBTHREAD_MUTEX_PP_RECURSIVE)
                ++mutex->counter;
            int ceiling = mutex->prioceiling;
            int status = change_muthread_priority(self, ceiling);
            if (status < 0)
                muprint("fail to change priority\n");
        }
        return ret;
    }
    return 0;
}

/* Unlock the mutex */
int muthread_mutex_unlock(muthread_mutex_t *mutex)
{
    muthread_t self = NULL;
    uint16_t type = mutex->type;
    switch(mutex->type){
    case TBTHREAD_MUTEX_NORMAL:
        if (atomic_exchange_explicit(&mutex->futex, 0, memory_order_release) == 2)
            SYSCALL3(__NR_futex, &mutex->futex, FUTEX_WAKE_PRIVATE, 1);
        return 0;
    case TBTHREAD_MUTEX_ERRORCHECK:
        if (mutex->owner != muthread_self() || mutex->futex == 0)
            return -EPERM;
        mutex->owner = 0;
        unlock_normal(mutex);
        return 0;
    case TBTHREAD_MUTEX_RECURSIVE:
        if (mutex->owner != muthread_self())
            return -EPERM;

        --mutex->counter;
        if (mutex->counter == 0) {
            mutex->owner = 0;
            unlock_normal(mutex);
        }
        return 0;
    case TBTHREAD_MUTEX_PI_NORMAL:
    case TBTHREAD_MUTEX_PI_ERRORCHECK:
    case TBTHREAD_MUTEX_PI_RECURSIVE:
        self = muthread_self();
        if (type == TBTHREAD_MUTEX_PI_ERRORCHECK && 
            (mutex->owner != self || mutex->futex == 0))
            return -EPERM;
        if (type == TBTHREAD_MUTEX_PI_RECURSIVE && mutex->owner != self)
            return -EPERM;

        if (type == TBTHREAD_MUTEX_PI_RECURSIVE) {
            --mutex->counter;
            if (mutex->counter == 0) {
                mutex->owner = 0;
                SYSCALL2(__NR_futex, mutex, FUTEX_UNLOCK_PI);
            }
        } else { 
            mutex->owner = 0;
            SYSCALL2(__NR_futex, mutex, FUTEX_UNLOCK_PI);
        }
        return 0;
    case TBTHREAD_MUTEX_PP_NORMAL:
    case TBTHREAD_MUTEX_PP_ERRORCHECK:
    case TBTHREAD_MUTEX_PP_RECURSIVE:
        self = muthread_self();
        if (type == TBTHREAD_MUTEX_PP_ERRORCHECK && 
            (mutex->owner != self || mutex->futex == 0))
            return -EPERM;
        if (type == TBTHREAD_MUTEX_PP_RECURSIVE && mutex->owner != self)
            return -EPERM;

        if (type == TBTHREAD_MUTEX_PP_RECURSIVE) {
            --mutex->counter;
            if (mutex->counter == 0) {
                mutex->owner = 0;
                unlock_normal(mutex);
            }
        } else {
            mutex->owner = 0;
            unlock_normal(mutex);
        }
        if (change_muthread_priority(self, (uint32_t) -1) < 0)
            muprint("fail to set priority to original\n");
        return 0;
    }
    return 0;
}