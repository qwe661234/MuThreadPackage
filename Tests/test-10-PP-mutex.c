#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include "mu.h"
#define THREADCOUNT 32

static muthread_mutex_t mutex_normal[10];
int resource_count = 1;

void TASK(int i)
{
    muthread_t self = muthread_self();
    int res = (i % resource_count) + 1;
    for(int j = 0; j < res; j++) {
        muthread_mutex_lock(&mutex_normal[j]);
        muprint("0x%lu GET RES %d\n", self, j);
    }
    for(int j = res - 1; j >= 0; j--) {
        muthread_mutex_unlock(&mutex_normal[j]);
    }
}

int main() {
    muthread_t th[THREADCOUNT];
    muthread_attr_t attr;
    muthread_mutexattr_t mattr;
    muthread_attr_init(&attr);
    struct sched_param param;
    muthread_mutexattr_setprioceiling(&mattr, THREADCOUNT < 100 ? THREADCOUNT : 99);
    muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_PROTECT);

    for(int i = 0; i < 10; i++) {
        muthread_mutex_init(&mutex_normal[i], &mattr);
    }

    for(int j = 1; j<= 10; j++) {
        resource_count = j;
        muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
        for(int i = 1; i <= THREADCOUNT; i++) {
            param.sched_priority = i < 100 ? i : 99;
            muthread_attr_setschedparam(&attr, &param);
            muthread_create(&th[i], &attr, (void *)TASK, i);
        } 
    }
    musleep(2);
    return 0;
}