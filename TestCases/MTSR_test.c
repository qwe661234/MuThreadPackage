#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include "mu.h"
#define THREADCOUNT 10

static muthread_mutex_t mutex_normal;


void TASK()
{
    muthread_t self = muthread_self();
    muthread_mutex_lock(&mutex_normal);
    muprint("0x%lu GET RES\n", self);
    muthread_mutex_unlock(&mutex_normal);
}

int main() {
    muthread_t th[THREADCOUNT];
    muthread_attr_t attr;
    muthread_mutexattr_t mattr;
    struct sched_param param;
    /* for priority inheritance mutex */
    muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_INHERIT);
    /* for priority ceiling mutex */
    // muthread_mutexattr_setprioceiling(&mattr, THREADCOUNT < 100 ? THREADCOUNT : 99);
    // muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_PROTECT);

    muthread_mutex_init(&mutex_normal, &mattr);
    muthread_attr_init(&attr);
    muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
    
    for(int i = 1; i <= THREADCOUNT; i++) {
        param.sched_priority = i < 100 ? i : 99;
        muthread_attr_setschedparam(&attr, &param);
        muthread_create(&th[i], &attr, (void *)TASK, NULL);
    }
    musleep(2);
    return 0;
}