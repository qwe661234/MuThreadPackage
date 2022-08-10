#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include "mu.h"
#define THREADCOUNT 10
#define RESCOUNT 3

static muthread_mutex_t mutex_normal[RESCOUNT];

void TASK(int n)
{
    muthread_t self = muthread_self();
    for (int j = 0; j < 1000; j++) {
        for (int i = 0; i < RESCOUNT; i++) {
            muthread_mutex_trylock(&mutex_normal[i]);
            muthread_mutex_unlock(&mutex_normal[i]);
        }
    }
}

int main() {
    muthread_t th[THREADCOUNT];
    muthread_attr_t attr;
    muthread_mutexattr_t mattr;
    muthread_attr_init(&attr);
    struct sched_param param;
    muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_PROTECT);

    for (int i = 0; i < RESCOUNT; i++) {
        muthread_mutexattr_setprioceiling(&mattr, THREADCOUNT + 10);
        muthread_mutex_init(&mutex_normal[i], &mattr);
    }

    muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
    for (int i = 0; i < THREADCOUNT; i++) {
        param.sched_priority = i + 1 < 100 ? i + 1 : 99;
        muthread_attr_setschedparam(&attr, &param);
        muthread_create(&th[i], &attr, (void *)TASK, NULL);
    }
    for(int i = 0; i < THREADCOUNT; i++) {
        muthread_join(th[i], NULL);
    }
    return 0;
}