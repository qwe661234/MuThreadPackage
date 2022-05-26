#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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
    pthread_mutex_t *mutex = (pthread_mutex_t *) arg;
	pthread_mutex_lock(mutex);
    for(int i = 0; i < 10; i++)
	    count++;
	pthread_mutex_unlock(mutex);
    return 0;
}

int main() {
    int st;
    pthread_t th[THREADCOUNT];
    pthread_mutex_t mutex_normal = PTHREAD_MUTEX_INITIALIZER;

    long long start = get_nanotime();
    for (int i = 0; i < THREADCOUNT; ++i) {
        st = pthread_create(&th[i], NULL, add, &mutex_normal);
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