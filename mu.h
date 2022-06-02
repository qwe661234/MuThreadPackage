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

/* Mutex types */
enum {
    TBTHREAD_MUTEX_NORMAL = 0,
    TBTHREAD_MUTEX_ERRORCHECK,
    TBTHREAD_MUTEX_RECURSIVE,
    TBTHREAD_MUTEX_DEFAULT = 0,
};

/* Thread attirbutes */
typedef struct {
    uint32_t stack_size;
    uint16_t policy;
    struct sched_param *param;
} muthread_attr_t;

/* Thread descriptor */
typedef struct muthread {
    struct muthread *self;
    void *stack;
    uint32_t stack_size;
    void *(*fn)(void *);
    void *arg;
    uint32_t tid;
    uint16_t policy;
    struct sched_param *param;
} * muthread_t;

/* Mutex attributes */
typedef struct {
    uint8_t type;
} muthread_mutexattr_t;

/* Mutex */
typedef struct {
    _Atomic int futex;
    uint8_t type;
    muthread_t owner;
    uint64_t counter;
} muthread_mutex_t;

#define TBTHREAD_MUTEX_INITIALIZER \
    {                              \
        0, 0, 0, 0                 \
    }

/* General threading */
void muthread_attr_init(muthread_attr_t *attr);
int muthread_attr_setschedpolicy(muthread_attr_t *attr, uint16_t policy);
int muthread_attr_setschedparam(muthread_attr_t *attr, struct sched_param *param);

int muthread_create(muthread_t *thread,
                    const muthread_attr_t *attrs,
                    void *(*f)(void *),
                    void *arg);

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
