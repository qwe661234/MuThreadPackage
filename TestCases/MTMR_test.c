#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include "mu.h"
#define THREADCOUNT 10
#define RESCOUNT 3

static muthread_mutex_t mutex_normal[RESCOUNT];


void TASK(int i)
{
    muthread_t self = muthread_self();
    int res = (i % RESCOUNT) + 1;
    for(int i = 0; i < res; i++) {
        muthread_mutex_lock(&mutex_normal[i]);
        muprint("0x%lu GET RES %d\n", self, i);
    }
    for(int i = res - 1; i >= 0; i--) {
        muthread_mutex_unlock(&mutex_normal[i]);
    }
}

int main() {
    muthread_t th[THREADCOUNT];
    muthread_attr_t attr;
    muthread_mutexattr_t mattr;
    muthread_attr_init(&attr);
    struct sched_param param;
    /* for priority inheritance mutex */
    muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_INHERIT);
    /* for priority ceiling mutex */
    // muthread_mutexattr_setprioceiling(&mattr, THREADCOUNT < 100 ? THREADCOUNT : 99);
    // muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_PROTECT);

    for(int i = 0; i < RESCOUNT; i++) {
        muthread_mutex_init(&mutex_normal[i], &mattr);
    }

    muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
    for(int i = 1; i <= THREADCOUNT; i++) {
        param.sched_priority = i < 100 ? i : 99;
        muthread_attr_setschedparam(&attr, &param);
        muthread_create(&th[i], &attr, (void *)TASK, i);
    }
    musleep(2);
    return 0;
}