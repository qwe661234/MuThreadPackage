#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include "mu.h"
#define THREADCOUNT 3

static muthread_mutex_t mutex_normal, mutex_normal_2;

void TASK1()
{
    muthread_mutex_lock(&mutex_normal);
    muprint("1\n");
    muthread_mutex_unlock(&mutex_normal);
}

void TASK2()
{ 
    muthread_mutex_lock(&mutex_normal_2);
    musleep(1);
    muprint("2\n");
    muthread_mutex_unlock(&mutex_normal_2);
}

void TASK3()
{   
    muthread_mutex_lock(&mutex_normal);
    musleep(1);
    muprint("3\n");
    muthread_mutex_unlock(&mutex_normal);
    
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
    musleep(2);
    return 0;
}