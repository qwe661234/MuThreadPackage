#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include "mu.h"
#define THREADCOUNT 3

static muthread_mutex_t mutex_normal, mutex_normal_2;


void Thread1()
{
    muthread_mutex_lock(&mutex_normal);
    muprint("1\n");
    muthread_mutex_unlock(&mutex_normal);
}

void Thread2()
{ 
    muthread_mutex_lock(&mutex_normal_2);
    musleep(1);
    muprint("2\n");
    muthread_mutex_unlock(&mutex_normal_2);
    
}

void Thread3()
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
    /* for muthread priority inheritance mutex */
    // muthread_mutexattr_settype(&mattr, TBTHREAD_MUTEX_PRIO_INHERIT);
    /* for pthread priority inheritance mutex */
    // pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
    // muthread_mutex_init(&mutex_normal, &mattr);
    // for normal mutex
    muthread_mutex_init(&mutex_normal, 0);
    muthread_mutex_init(&mutex_normal_2, 0);
    muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
    param.sched_priority = 10;
    muthread_attr_setschedparam(&attr, &param);
    muthread_create(&th[2], &attr, (void *)Thread3, NULL);
    param.sched_priority = 20;
    muthread_attr_setschedparam(&attr, &param);
    muthread_create(&th[1], &attr, (void *)Thread2, NULL);
    param.sched_priority = 30;
    muthread_attr_setschedparam(&attr, &param);
    muthread_create(&th[0], &attr, (void *)Thread1, NULL);
    musleep(2);
    return 0;
}