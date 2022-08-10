#include "mu.h"
#include <stdlib.h>
#include <string.h>

#define THREADCOUNT 8

muthread_cond_t c; 
int count = 0;
muthread_mutex_t mutex_normal;
muthread_mutexattr_t mattr;
muthread_attr_t attr;

void *task (void *arg) {
    muthread_mutex_lock(&mutex_normal);
    count++;
    while(count != THREADCOUNT) 
        muthread_cond_wait(&c, &mutex_normal);
    muprint("Thread 0x%lx complete task\n", muthread_self());
    muthread_cond_signal(&c, &mutex_normal);
    muthread_mutex_unlock(&mutex_normal);
}

int main() {
    muthread_t th[THREADCOUNT];
    muthread_mutex_init(&mutex_normal, 0);
    muthread_attr_init(&attr);
    for (int i = 0; i < THREADCOUNT; i++) {
        muthread_create(&th[i], &attr, task, NULL);
    }
    for (int i = 0; i < THREADCOUNT; i++) {
        muthread_join(th[i], NULL);
    }
    muprint("Test End!!!\n");
    return 0;
}