#include "mu.h"
#include <stdlib.h>
#include <string.h>

#define THREADCOUNT 8

muthread_cond_t c; 
int count = 0;
muthread_mutex_t mutex_normal;
muthread_attr_t attr;
muthread_mutexattr_t mattr;

void *task (void *arg) {
    struct sched_param param;
    muthread_mutex_lock(&mutex_normal);
    count++;
    while(count != THREADCOUNT) 
        muthread_cond_wait_pi(&c, &mutex_normal);
    sched_getparam(muthread_self()->tid, &param);
    muprint("Thread 0x%lx with priority = %d complete task\n", muthread_self(), param.sched_priority);
    muthread_cond_signal_pi(&c, &mutex_normal);
    muthread_mutex_unlock(&mutex_normal);
}

int main() {
    muthread_t th[THREADCOUNT];
    struct sched_param param;
    muthread_attr_init(&attr);
    muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
    muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_INHERIT);
    muthread_mutex_init(&mutex_normal, &mattr);
    for (int i = 0; i < THREADCOUNT; i++) {
        param.sched_priority = (i + 1)  * 10;
        muthread_attr_setschedparam(&attr, &param);
        muthread_create(&th[i], &attr, task, NULL);
    }
    for (int i = 0; i < THREADCOUNT; i++) {
        muthread_join(th[i], NULL);
    }
    muprint("Test End!!!\n");
    return 0;
}