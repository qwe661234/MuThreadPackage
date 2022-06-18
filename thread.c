#include "mu.h"

#include <asm-generic/mman-common.h>
#include <asm-generic/param.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <sys/types.h>
#include <sched.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Initialize the attrs to the defaults */
void muthread_attr_init(muthread_attr_t *attr)
{
    attr->stack_size = 8192 * 1024;
    attr->policy = SCHED_OTHER;
    attr->param = NULL;
}

int muthread_attr_setschedpolicy(muthread_attr_t *attr, uint16_t policy)
{
    if (policy < SCHED_OTHER || policy > SCHED_DEADLINE)
        return -EINVAL;
    attr->policy = policy;
    return 0; 
}

int muthread_attr_setschedparam(muthread_attr_t *attr, struct sched_param *param)
{
    if (!param)
        return -EINVAL;
    uint16_t prio_max = SYSCALL1(__NR_sched_get_priority_max, attr->policy);
    uint16_t prio_min = SYSCALL1(__NR_sched_get_priority_min, attr->policy);
    if (param->sched_priority > prio_max || param->sched_priority < prio_min)
        return -EINVAL;
    attr->param = param;
    return 0; 
}

int muthread_attr_setinheritsched(muthread_attr_t *attr, int inherit) 
{
    if (inherit != TBTHREAD_INHERIT_SCHED && inherit != TBTHREAD_EXPLICIT_SCHED)
        return EINVAL;
    attr->flags = inherit;
    return 0;
}
/* Thread function wrapper */
static int start_thread(void *arg)
{
    muthread_t th = (muthread_t) arg;
    uint32_t stack_size = th->stack_size;
    void *stack = th->stack;
    th->fn(th->arg);
    free(th);

    /* Free the stack and exit. We do it this way because we remove the stack
     * from underneath our feet and cannot allow the C code to write on it
     * anymore.
     */
    register long a1 asm("rdi") = (long) stack;
    register long a2 asm("rsi") = stack_size;
    asm volatile(
        "syscall\n\t"
        "movq $60, %%rax\n\t"  // 60 = __NR_exit
        "movq $0, %%rdi\n\t"
        "syscall"
        :                                     // Output
        : "a"(__NR_munmap), "r"(a1), "r"(a2)  // Input
        : "memory", "cc", "r11", "cx");       // Clobber List
        /* Clobber list -> Each string is the name of a register that the assembly code  
         * potentially modifies, but for which the final value is not important
         * "memory": if instruction modifies memory
         * "cc": if instruction can alter the condition code register 
         */
    return 0;
}

/* Spawn a thread */
int muthread_create(muthread_t *thread,
                    const muthread_attr_t *attr,
                    void *(*f)(void *),
                    void *arg)
{
    /* Allocate the stack with a guard page at the end so that we could protect
     * from overflows (by receiving a SIGSEGV)
     */
    void *stack = mummap(NULL, attr->stack_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    long status = (long) stack;
    if (status < 0)
        return status;

    status = SYSCALL3(__NR_mprotect, stack, EXEC_PAGESIZE, PROT_NONE);
    if (status < 0) {
        mumunmap(stack, attr->stack_size);
        return status;
    }

    /* Pack everything up */
    *thread = malloc(sizeof(struct muthread));
    memset(*thread, 0, sizeof(struct muthread));
    (*thread)->self = *thread;
    (*thread)->stack = stack;
    (*thread)->stack_size = attr->stack_size;
    (*thread)->policy = attr->policy;
    (*thread)->fn = f;
    (*thread)->arg = arg;
    (*thread)->wait_list_lock = 0;
    (*thread)->priority_lock = 0;
    if (attr->param) {
        (*thread)->param.sched_priority = attr->param->sched_priority;
    }
    (*thread)->list = NULL;
    /* Spawn the thread */
    int flags =
        CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM | CLONE_SIGHAND;
    flags |= CLONE_THREAD | CLONE_SETTLS;
    
    /* Because the address of stack is from high to low, so the pointer to 
     * child stack should be stack + attr->size 
     */
    int tid = muclone(start_thread, *thread, flags,
                      (char *) stack + attr->stack_size, 0, 0, *thread);
    if (tid < 0) {
        mumunmap(stack, attr->stack_size);
        free(*thread);
        return tid;
    }

    (*thread)->tid = tid;
    if (attr->flags == TBTHREAD_EXPLICIT_SCHED) {
        if(!attr->param || !(*thread)->policy) {
            muprint("sched parameter or policy is NULL \n");
            return -1;
        }
        status = SYSCALL3(__NR_sched_setscheduler, (*thread)->tid, (*thread)->policy, &(*thread)->param);
        if (status < 0) {
            muprint("fail to set scheduler \n");
            return status;
        }
    }
    return 0;
}