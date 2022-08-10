#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <semaphore.h>
#include <time.h>
#include "mu.h"

// contact: attractor@live.co.uk

// http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
// http://www.alexonlinux.com/multithreaded-simple-data-type-access-and-atomic-variables

typedef struct {
	int n, m, type, start, step;
	uint64_t *bits;
} worker_t;

#define slow_comp(i, n) ((uint64_t)(i) * (i) * (i) % (n))

static muthread_mutex_t g_mutex;
static volatile uint8_t g_lock2;

static inline long long get_nanotime()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

void *worker(void *data)
{
	worker_t *w = (worker_t*)data;
	int64_t i, nm;
	
	for (i = w->start, nm = (int64_t)w->n * w->m; i < nm; i += w->step) {
		uint64_t x = slow_comp(i, w->n);
		uint64_t *p = &w->bits[x>>6];
		uint64_t y = 1LLU << (x & 0x3f);
		muthread_mutex_lock(&g_mutex);
		*p ^= y;
		muthread_mutex_unlock(&g_mutex);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int c, n_threads = 1, i, tmp, st;
	uint64_t z;
	worker_t *w, w0;
	muthread_t *tid;
	muthread_attr_t attr;

	w0.n = 1000000; w0.m = 100; w0.type = 1; w0.start = 0; w0.step = 1;
	while ((c = getopt(argc, argv, "t:n:s:m:l:")) >= 0) {
		switch (c) {
			case 't': n_threads = atoi(optarg); break;
			case 'n': w0.n = atoi(optarg); break;
			case 'm': w0.m = atoi(optarg); break;
			case 'l': w0.type = atoi(optarg); break;
		}
	}
	fprintf(stderr, "Usage: lock_test [-t nThreads=%d] [-n size=%d] [-m repeat=%d] [-l lockType=%d]\n",
			n_threads, w0.n, w0.m, w0.type);
	fprintf(stderr, "Lock type: 0 for mutex;\n");
	
	/* 64 align */
	w0.bits = (uint64_t*)malloc(((w0.n + 63) / 64) * 8);

	muthread_mutex_init(&g_mutex, 0);
	muthread_attr_init(&attr);
	tid = malloc(sizeof(muthread_t) * n_threads);
	w = malloc(sizeof(worker_t) * n_threads);
	for (i = 0; i < n_threads; ++i) {
		w[i] = w0;
		w[i].start = i; w[i].step = n_threads;
	}
	long long start = get_nanotime();
	for (i = 0; i < n_threads; ++i) {
		st = muthread_create(&tid[i], &attr, worker, &w[i]);
		if (st != 0) {
            muprint("Failed to spawn thread %d: %s\n", i, strerror(-st));
            return 1;
        }
	}
	
	for (i = 0; i < n_threads; ++i) muthread_join(tid[i], NULL);

	long long utime = get_nanotime() - start;
	for (i = 0, z = 0, tmp = (w0.n + 63)/64; i < tmp; ++i)
		z ^= w0.bits[i];
	fprintf(stderr, "Hash: %llx (should be 0 if -m is even or aaaaaaaaaaaaaaa if -m is odd)\n", (unsigned long long)z);
	fprintf(stderr, "time = %llu\n", utime);
	free(w0.bits);
	return 0;
}
