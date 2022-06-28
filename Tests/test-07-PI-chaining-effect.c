#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include "mu.h"
#define THREADCOUNT 3

static muthread_mutex_t mutex_normal, mutex_normal_2;


void TASK1()
{
    muthread_mutex_lock(&mutex_normal_2);
    muthread_mutex_unlock(&mutex_normal_2);
}

void TASK2()
{ 
    struct sched_param param;
    muthread_mutex_lock(&mutex_normal_2);
    muthread_mutex_lock(&mutex_normal);
    sched_getparam(muthread_self()->tid, &param);
    muprint("TASK2 current priority = %d\n", param.sched_priority);
    muthread_mutex_unlock(&mutex_normal);
    sched_getparam(muthread_self()->tid, &param);
    muprint("TASK2 current priority = %d\n", param.sched_priority);
    muthread_mutex_unlock(&mutex_normal_2);
    sched_getparam(muthread_self()->tid, &param);
    muprint("TASK2 current priority = %d\n", param.sched_priority);
}

void TASK3()
{   
    struct sched_param param;
    muthread_mutex_lock(&mutex_normal);
    musleep(1);
    sched_getparam(muthread_self()->tid, &param);
    muprint("TASK3 current priority = %d\n", param.sched_priority);
    muthread_mutex_unlock(&mutex_normal);
    sched_getparam(muthread_self()->tid, &param);
    muprint("TASK3 current priority = %d\n", param.sched_priority);
}

static void (*TASKS[])() = {
    TASK1,
    TASK2,
    TASK3,
};

int main() {
    int st;
    muthread_t th[THREADCOUNT];
    muthread_attr_t attr;
    muthread_mutexattr_t mattr;
    muthread_attr_init(&attr);
    struct sched_param param;
    muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_INHERIT);
    muthread_mutex_init(&mutex_normal, &mattr);
    muthread_mutex_init(&mutex_normal_2, &mattr);
    muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
    for(int i = THREADCOUNT - 1; i >= 0; i--) {
        param.sched_priority = (THREADCOUNT - i) * 10;
        muthread_attr_setschedparam(&attr, &param);
        st = muthread_create(&th[i], &attr, (void *)TASKS[i], NULL);
        if (st != 0) {
            muprint("Failed to spawn thread %d: %s\n", i, strerror(-st));
            return 1;
        }
    }
    musleep(3);
    return 0;
}