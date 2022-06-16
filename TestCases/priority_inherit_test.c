#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include "mu.h"
#define THREADCOUNT 3

static muthread_mutex_t mutex_normal, mutex_normal_2;


void Thread_H()
{
    muthread_mutex_lock(&mutex_normal_2);
    muprint("1\n");
    muthread_mutex_unlock(&mutex_normal_2);
}

void Thread_M()
{ 
    muthread_mutex_lock(&mutex_normal_2);
    muthread_mutex_lock(&mutex_normal);
    musleep(1);
    muprint("2\n");
    muthread_mutex_unlock(&mutex_normal);
    muthread_mutex_unlock(&mutex_normal_2);
    
}

void Thread_L()
{   
    muthread_mutex_lock(&mutex_normal);
    musleep(1);
    muprint("3\n");
    muthread_mutex_unlock(&mutex_normal);
}

int main() {
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
    param.sched_priority = 10;
    muthread_attr_setschedparam(&attr, &param);
    muthread_create(&th[2], &attr, (void *)Thread_L, NULL);
    param.sched_priority = 20;
    muthread_attr_setschedparam(&attr, &param);
    muthread_create(&th[1], &attr, (void *)Thread_M, NULL);
    param.sched_priority = 30;
    muthread_attr_setschedparam(&attr, &param);
    muthread_create(&th[0], &attr, (void *)Thread_H, NULL);
    musleep(3);
    return 0;
}