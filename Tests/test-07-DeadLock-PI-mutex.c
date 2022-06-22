#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include "mu.h"
static muthread_mutex_t mutex_normal, mutex_normal_2;

void TASK1()
{
    muthread_mutex_lock(&mutex_normal_2);
    muprint("2 GET 2\n");
    muthread_mutex_lock(&mutex_normal);
    muprint("2 GET 1\n");
    muthread_mutex_unlock(&mutex_normal);
    muthread_mutex_unlock(&mutex_normal_2);
}

void TASK2()
{   
    muthread_mutex_lock(&mutex_normal);
    muprint("1 GET 1\n");
    muthread_mutex_lock(&mutex_normal_2);
    muprint("1 GET 2\n");
    muthread_mutex_unlock(&mutex_normal_2);
    muthread_mutex_unlock(&mutex_normal);
}

int main(int argc, char *argv[])
{
    muthread_t th[2];
    muthread_attr_t attr;
    muthread_mutexattr_t mattr;
    muthread_attr_init(&attr);
    struct sched_param param;

    /* for priority inheritance mutex */
    // muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_INHERIT);
    /* for priority ceiling mutex */
    muthread_mutexattr_setprioceiling(&mattr, 30);
    muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_PROTECT);

    muthread_mutex_init(&mutex_normal, &mattr);
    muthread_mutex_init(&mutex_normal_2, &mattr);

    muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
    param.sched_priority = 10;
    muthread_attr_setschedparam(&attr, &param);
    muthread_create(&th[0], &attr, (void *)TASK1, NULL);
    param.sched_priority = 20;
    muthread_attr_setschedparam(&attr, &param);
    muthread_create(&th[1], &attr, (void *)TASK2, NULL);
    musleep(2);
    return 0;
}