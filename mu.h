#pragma once

#if !defined(__x86_64__) || !defined(__linux__)
#error "This package only supports Linux/x86_64"
#endif

#include <asm-generic/errno.h>
#include <asm/unistd_64.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <sched.h>
#include <unistd.h>

/*
 * pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol);
 * In pthread library, priority inheritance / protection mutex should be set 
 * by funciton pthread_mutexattr_setprotocol, and set its second parameter as 
 * PTHREAD_PRIO_INHERIT / PTHREAD_PRIO_PROTECT.
 */

#ifdef PTHREAD
#include <pthread.h>
#include <stdio.h>
#define muthread_t pthread_t
#define muthread_attr_t pthread_attr_t
#define muthread_mutexattr_t pthread_mutexattr_t
#define muthread_mutex_t pthread_mutex_t
#define muthread_attr_init pthread_attr_init
#define muthread_attr_setschedparam pthread_attr_setschedparam
#define muthread_attr_setschedpolicy pthread_attr_setschedpolicy
#define muthread_attr_setinheritsched pthread_attr_setinheritsched
#define muthread_create pthread_create
#define muthread_self pthread_self
#define muthread_join pthread_join
#define muthread_mutexattr_init pthread_mutexattr_init
#define muthread_mutexattr_settype pthread_mutexattr_settype
#define muthread_mutexattr_setprotocol pthread_mutexattr_setprotocol
#define muthread_mutexattr_setprioceiling pthread_mutexattr_setprioceiling
#define muthread_mutex_init pthread_mutex_init
#define muthread_mutex_lock pthread_mutex_lock
#define muthread_mutex_trylock pthread_mutex_trylock
#define muthread_mutex_unlock pthread_mutex_unlock
#define TBTHREAD_MUTEX_NORMAL TBTHREAD_MUTEX_NORMAL
#define TBTHREAD_MUTEX_ERRORCHECK PTHREAD_MUTEX_ERRORCHECK
#define TBTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE
#define TBTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_DEFAULT
#define TBTHREAD_PRIO_NONE PTHREAD_PRIO_NONE
#define TBTHREAD_PRIO_INHERIT PTHREAD_PRIO_INHERIT
#define TBTHREAD_PRIO_PROTECT PTHREAD_PRIO_PROTECT
#define TBTHREAD_INHERIT_SCHED PTHREAD_INHERIT_SCHED
#define TBTHREAD_EXPLICIT_SCHED PTHREAD_EXPLICIT_SCHED
#define muprint printf
#define musleep sleep
#else
/* Mutex types */
enum {
    TBTHREAD_MUTEX_NORMAL = 0,
    TBTHREAD_MUTEX_ERRORCHECK,
    TBTHREAD_MUTEX_RECURSIVE,
    TBTHREAD_MUTEX_DEFAULT = 0,
};

/* Mutex attribute's protocol */
enum {
    TBTHREAD_PRIO_NONE,
    TBTHREAD_PRIO_INHERIT = 3,
    TBTHREAD_PRIO_PROTECT = 4,
};

/* Scheduler inheritance */
enum {
    TBTHREAD_INHERIT_SCHED,
    TBTHREAD_EXPLICIT_SCHED,
};
/* Thread attirbutes */
typedef struct {
    uint32_t stack_size;
    uint16_t policy;
    uint16_t flags;
    struct sched_param *param;
} muthread_attr_t;

 
/* Thread descriptor */
struct priority_protection_data {
  int priomax;
  uint32_t priomap[99];
};

typedef struct wait_list wait_list_t;

typedef struct muthread {
    struct muthread *self;
    void *stack;
    uint32_t stack_size;
    void *(*fn)(void *);
    void *arg;
    uint32_t tid;
    uint16_t policy;
    struct sched_param param;
    _Atomic int priority_lock;
    _Atomic int wait_list_lock;
    wait_list_t *list;
    struct priority_protection_data tpp;
} * muthread_t;

/* Mutex attributes */
typedef struct {
    uint16_t type;
} muthread_mutexattr_t;

/* Mutex */
typedef struct {
    _Atomic int futex;
    /* bit 0 ~ 7: prioceiling, bit 8 ~ 11: protocol, bit 12 ~ 15: mutex type */
    uint16_t type;
    muthread_t owner;
    uint64_t counter;
} muthread_mutex_t;

struct wait_list {
    muthread_mutex_t *mutex;
    struct wait_list *next;
};

#define TBTHREAD_MUTEX_INITIALIZER \
    {                              \
        0, 0, 0, 0                 \
    }

/* General threading */
void muthread_attr_init(muthread_attr_t *attr);
int muthread_attr_setschedpolicy(muthread_attr_t *attr, uint16_t policy);
int muthread_attr_setschedparam(muthread_attr_t *attr, struct sched_param *param);
int muthread_attr_setinheritsched(muthread_attr_t *attr, int inherit);

int muthread_create(muthread_t *thread,
                    const muthread_attr_t *attrs,
                    void *(*f)(void *),
                    void *arg);
void muthread_join(muthread_t thread, void **thread_return);
/* Get the pointer of the currently running thread */
static inline muthread_t muthread_self()
{
    muthread_t self;
    /* The fs register is commonly used to address Thread Local Storage (TLS) */
    asm("movq %%fs:0, %0\n\t" : "=r"(self));
    return self;
}

/* Mutexes */
int muthread_mutexattr_init(muthread_mutexattr_t *attr);
int muthread_mutexattr_settype(muthread_mutexattr_t *attr, int type);
int muthread_mutexattr_setprotocol(muthread_mutexattr_t *attr, int protocol);
int muthread_mutexattr_setprioceiling(muthread_mutexattr_t *attr, int prioceiling);

int muthread_mutex_init(muthread_mutex_t *mutex,
                        const muthread_mutexattr_t *attr);
int muthread_mutex_lock(muthread_mutex_t *mutex);
int muthread_mutex_trylock(muthread_mutex_t *mutex);
int muthread_mutex_unlock(muthread_mutex_t *mutex);

/* Utility functions */
void muprint(const char *format, ...);
void musleep(int secs);
void *mummap(void *addr,
             unsigned long length,
             int prot,
             int flags,
             int fd,
             unsigned long offset);
int mumunmap(void *addr, unsigned long length);

int muclone(int (*fn)(void *), void *arg, int flags, void *child_stack, ...
            /* pid_t *ptid, pid_t *ctid */);
#endif
/* Syscall interface */
#define SYSCALL(name, a1, a2, a3, a4, a5, a6)                             \
    ({                                                                    \
        long result;                                                      \
        long __a1 = (long) (a1), __a2 = (long) (a2), __a3 = (long) (a3);  \
        long __a4 = (long) (a4), __a5 = (long) (a5), __a6 = (long) (a6);  \
        register long _a1 asm("rdi") = __a1;                              \
        register long _a2 asm("rsi") = __a2;                              \
        register long _a3 asm("rdx") = __a3;                              \
        register long _a4 asm("r10") = __a4;                              \
        register long _a5 asm("r8") = __a5;                               \
        register long _a6 asm("r9") = __a6;                               \
        asm volatile("syscall\n\t"                                        \
                     : "=a"(result)                                       \
                     : "0"(name), "r"(_a1), "r"(_a2), "r"(_a3), "r"(_a4), \
                       "r"(_a5), "r"(_a6)                                 \
                     : "memory", "cc", "r11", "cx");                      \
        (long) result;                                                    \
    })

#define SYSCALL1(name, a1) SYSCALL(name, a1, 0, 0, 0, 0, 0)
#define SYSCALL2(name, a1, a2) SYSCALL(name, a1, a2, 0, 0, 0, 0)
#define SYSCALL3(name, a1, a2, a3) SYSCALL(name, a1, a2, a3, 0, 0, 0)
#define SYSCALL4(name, a1, a2, a3, a4) SYSCALL(name, a1, a2, a3, a4, 0, 0)
#define SYSCALL5(name, a1, a2, a3, a4, a5) SYSCALL(name, a1, a2, a3, a4, a5, 0)
#define SYSCALL6(name, a1, a2, a3, a4, a5, a6) \
    SYSCALL(name, a1, a2, a3, a4, a5, a6)

#define ___PASTE(a , b) a##b
#define __PASTE(a , b) ___PASTE(a, b)
#define __UNIQUE_ID(prefix) \
    __PASTE(__PASTE(__UNIQUE_ID_, prefix), __COUNTER__)
#define _atomic_bool_cmpxchg(ptr, varname1, varname2, old, new)            \
    ({                                                                     \
        typeof(*ptr) varname1 = (old), varname2 = (new);                   \
        bool r = atomic_compare_exchange_strong_explicit(                  \
            ptr, &varname1, varname2, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
        r;                                                                 \
    })
#define atomic_bool_cmpxchg(ptr, old, new)                                          \
            _atomic_bool_cmpxchg(ptr, __UNIQUE_ID(old), __UNIQUE_ID(new), old, new) \

#ifndef PTHREAD
int change_muthread_priority(muthread_t target, uint32_t priority, int is_inherit, int is_protect);
void wait_list_add(muthread_mutex_t *resource);
void wait_list_delete(muthread_mutex_t *resource);
int inherit_priority_chaining(muthread_t list_owner, uint32_t priority);
#endif