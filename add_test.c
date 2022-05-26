#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "mu.h"
#define THREADCOUNT 20

static inline long long get_nanotime()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int count = 0;
void *add (void *arg){
    muthread_t self = muthread_self();
    muthread_mutex_t *mutex = (muthread_mutex_t *) arg;
	muthread_mutex_lock(mutex);
    for(int i = 0; i < 10; i++)
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

    long long start = get_nanotime();
    for (int i = 0; i < THREADCOUNT; ++i) {
        st = muthread_create(&th[i], &attr, add, &mutex_normal);
        if (st != 0) {
            muprint("Failed to spawn thread %d: %s\n", i, strerror(-st));
            return 1;
        }
    }
    long long utime = get_nanotime() - start;
    musleep(1);
    muprint("count = %d\n", count);
    muprint("time = %lld\n", utime);
}