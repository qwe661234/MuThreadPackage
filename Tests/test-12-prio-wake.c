#include "mu.h"
#include <stdlib.h>
#include <string.h>

#define THREADCOUNT 8

muthread_cond_t c; 
int count = 0;
muthread_mutex_t mutex_normal;
muthread_mutexattr_t mattr;
muthread_attr_t attr;

void *master_thread(void *arg)
{
	/* make sure children are started */
	while (count < THREADCOUNT)
		usleep(1000);
	/* give the worker threads a chance to get to sleep in the kernel
	 * in the unlocked broadcast case. */
	usleep(1000);

	// printf("Master thread about to wake the workers\n");
	/* start the children threads */
    muthread_mutex_lock(&mutex_normal);
	muthread_cond_broadcast_pi(&c, &mutex_normal);
    muthread_mutex_unlock(&mutex_normal);
	return NULL;
}

void *task (void *arg) {
    struct sched_param param;
    muthread_mutex_lock(&mutex_normal);
    sched_getparam(muthread_self()->tid, &param);
    muprint("Thread 0x%lx with priority = %d started\n", muthread_self(), param.sched_priority);
    count++;
    muthread_cond_wait_pi(&c, &mutex_normal);
    sched_getparam(muthread_self()->tid, &param);
    muprint("Thread 0x%lx with priority = %d complete task\n", muthread_self(), param.sched_priority);
    muthread_mutex_unlock(&mutex_normal);
    return NULL;
}

int main() {
    muthread_t th[THREADCOUNT], master;
    struct sched_param param;
    muthread_attr_init(&attr);
    muthread_attr_setinheritsched(&attr, TBTHREAD_EXPLICIT_SCHED);
    muthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    muthread_mutexattr_setprotocol(&mattr, TBTHREAD_PRIO_INHERIT);
    muthread_mutex_init(&mutex_normal, &mattr);
    muthread_create(&master, &attr, master_thread, NULL);
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