#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "mu.h"
#define THREADCOUNT 20
unsigned long long count = 0;

void *add (void *arg){
    muthread_mutex_t *mutex = (muthread_mutex_t *) arg;
	muthread_mutex_lock(mutex);
    for(unsigned long long i = 0; i < 100000; i++)
	    count++;
	muthread_mutex_unlock(mutex);
    return 0;
}

int main() {
    int st;
    muthread_t th[THREADCOUNT];
    muthread_mutex_t mutex_normal;
    muthread_attr_t attr;
    muthread_attr_init(&attr);
    muthread_mutex_init(&mutex_normal, 0);

    for (int i = 0; i < THREADCOUNT; ++i) {
        st = muthread_create(&th[i], &attr, add, &mutex_normal);
        if (st != 0) {
            muprint("Failed to spawn thread %d: %s\n", i, strerror(-st));
            return 1;
        }
    }
    for (int i = 0; i < THREADCOUNT; ++i) {
        muthread_join(th[i], NULL);
    }
    muprint("count = %lld\n", count);
}