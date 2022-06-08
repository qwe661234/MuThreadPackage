#include "mu.h"

#include <linux/futex.h>
#include <sched.h>

/* Normal mutex */
static int lock_normal(muthread_mutex_t *mutex)
{
    while (1) {
        if (atomic_bool_cmpxchg(&mutex->futex, 0, 1))
            return 0;
        SYSCALL3(__NR_futex, &mutex->futex, FUTEX_WAIT, 1);
    }
}

static int trylock_normal(muthread_mutex_t *mutex)
{
    if (atomic_bool_cmpxchg(&mutex->futex, 0, 1))
        return 0;
    return -EBUSY;
}

static int unlock_normal(muthread_mutex_t *mutex)
{
    if (atomic_bool_cmpxchg(&mutex->futex, 1, 0))
        SYSCALL3(__NR_futex, &mutex->futex, FUTEX_WAKE, 1);
    return 0;
}

/* Errorcheck mutex */
static int lock_errorcheck(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
    if (mutex->owner == self)
        return -EDEADLK;

    lock_normal(mutex);
    mutex->owner = self;
    return 0;
}

static int trylock_errorcheck(muthread_mutex_t *mutex)
{
    if (mutex->owner == muthread_self())
        return -EDEADLK;

    int ret = trylock_normal(mutex);
    if (ret == 0)
        mutex->owner = muthread_self();
    return ret;
}

static int unlock_errorcheck(muthread_mutex_t *mutex)
{
    if (mutex->owner != muthread_self() || mutex->futex == 0)
        return -EPERM;
    mutex->owner = 0;
    unlock_normal(mutex);
    return 0;
}

/* Recursive mutex */
static int lock_recursive(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
    if (mutex->owner != self) {
        lock_normal(mutex);
        mutex->owner = self;
    }
    /* Check counter overflow */
    if (mutex->counter == (uint64_t) -1)
        return -EAGAIN;

    ++mutex->counter;
    return 0;
}

static int trylock_recursive(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
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
}

static int unlock_recursive(muthread_mutex_t *mutex)
{
    if (mutex->owner != muthread_self())
        return -EPERM;

    --mutex->counter;
    if (mutex->counter == 0) {
        mutex->owner = 0;
        return unlock_normal(mutex);
    }
    return 0;
}

/* Priority inheritance mutex*/
static int lock_priority_inherit(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
    if (mutex->owner) {
        struct sched_param param;
        SYSCALL2(__NR_sched_getparam, mutex->owner->tid, &param);
        if (param.sched_priority < self->param->sched_priority) {
            param.sched_priority = self->param->sched_priority;
            int status = SYSCALL3(__NR_sched_setscheduler, mutex->owner->tid, mutex->owner->policy, &param);
            if (status < 0) {
                muprint("fail to set scheduler \n");
                return status;
            }
        }
    }
    lock_normal(mutex);
    mutex->owner = self;
    return 0;
}

static int trylock_priority_inherit(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
    if (mutex->owner) {
        struct sched_param param;
        SYSCALL2(__NR_sched_getparam, mutex->owner->tid, &param);
        if (param.sched_priority < self->param->sched_priority) {
            param.sched_priority = self->param->sched_priority;
            int status = SYSCALL3(__NR_sched_setscheduler, mutex->owner->tid, mutex->owner->policy, &param);
            if (status < 0) {
                muprint("fail to set scheduler \n");
                return status;
            }
        }
    }
    trylock_normal(mutex);
    mutex->owner = self;
    return 0;
}

static int unlock_priority_inherit(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
    mutex->owner = 0;
    unlock_normal(mutex);
    struct sched_param param;
    SYSCALL2(__NR_sched_getparam, self->tid, &param);
    if (self->param->sched_priority != param.sched_priority) {
        int status = SYSCALL3(__NR_sched_setscheduler, self->tid, self->policy, self->param);
        if (status < 0) {
            muprint("fail to set scheduler \n");
            return status;
        }
    }
    return 0;
}

/* Priority protection mutex*/
static int lock_priority_protect(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
    lock_normal(mutex);
    int ceiling = SYSCALL1(__NR_sched_get_priority_max, self->policy);
    struct sched_param param;
    SYSCALL2(__NR_sched_getparam, self->tid, &param);
    if (param.sched_priority < ceiling) {
        param.sched_priority = ceiling;
        int status = SYSCALL3(__NR_sched_setscheduler, self->tid, self->policy, &param);
        if (status < 0) {
            muprint("fail to set scheduler \n");
            return status;
        }
    }
    return 0;
}

static int trylock_priority_protect(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
    trylock_normal(mutex);
    int ceiling = SYSCALL1(__NR_sched_get_priority_max, self->policy);
    struct sched_param param;
    SYSCALL2(__NR_sched_getparam, self->tid, &param);
    if (param.sched_priority < ceiling) {
        param.sched_priority = ceiling;
        int status = SYSCALL3(__NR_sched_setscheduler, self->tid, self->policy, &param);
        if (status < 0) {
            muprint("fail to set scheduler \n");
            return status;
        }
    }
    return 0;
}

static int unlock_priority_protect(muthread_mutex_t *mutex)
{
    muthread_t self = muthread_self();
    unlock_normal(mutex);
    struct sched_param param;
    SYSCALL2(__NR_sched_getparam, self->tid, &param);
    if (self->param->sched_priority != param.sched_priority) {
        int status = SYSCALL3(__NR_sched_setscheduler, self->tid, self->policy, self->param);
        if (status < 0) {
            muprint("fail to set scheduler \n");
            return status;
        }
    }
    return 0;
}
/* Mutex function tables */
static int (*lockers[])(muthread_mutex_t *) = {
    lock_normal,
    lock_errorcheck,
    lock_recursive,
    lock_priority_inherit,
    lock_priority_protect,
};

static int (*trylockers[])(muthread_mutex_t *) = {
    trylock_normal,
    trylock_errorcheck,
    trylock_recursive,
    trylock_priority_inherit,
    trylock_priority_protect,
};

static int (*unlockers[])(muthread_mutex_t *) = {
    unlock_normal,
    unlock_errorcheck,
    unlock_recursive,
    unlock_priority_inherit,
    unlock_priority_protect,
};

/* Init attributes */
int muthread_mutexattr_init(muthread_mutexattr_t *attr)
{
    attr->type = TBTHREAD_MUTEX_DEFAULT;
    return 0;
}

/* Set attributes */
int muthread_mutexattr_settype(muthread_mutexattr_t *attr, int type)
{
    if (type < TBTHREAD_MUTEX_NORMAL || type > TBTHREAD_MUTEX_PRIO_PROTECT)
        return -EINVAL;
    attr->type = type;
    return 0;
}

/* Initialize the mutex */
int muthread_mutex_init(muthread_mutex_t *mutex,
                        const muthread_mutexattr_t *attr)
{
    uint8_t type = TBTHREAD_MUTEX_DEFAULT;
    if (attr)
        type = attr->type;
    mutex->futex = 0;
    mutex->type = type;
    mutex->owner = 0;
    mutex->counter = 0;
    return 0;
}

/* Lock the mutex */
int muthread_mutex_lock(muthread_mutex_t *mutex)
{
    return (*lockers[mutex->type])(mutex);
}

/* Try locking the mutex */
int muthread_mutex_trylock(muthread_mutex_t *mutex)
{
    return (*trylockers[mutex->type])(mutex);
}

/* Unlock the mutex */
int muthread_mutex_unlock(muthread_mutex_t *mutex)
{
    return (*unlockers[mutex->type])(mutex);
}