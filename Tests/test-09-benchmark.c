#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>

// contact: attractor@live.co.uk

// http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
// http://www.alexonlinux.com/multithreaded-simple-data-type-access-and-atomic-variables

typedef struct {
	int n, m, type, start, step;
	uint64_t *bits;
} worker_t;

#define slow_comp(i, n) ((uint64_t)(i) * (i) * (i) % (n))

static pthread_mutex_t g_mutex;
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
		pthread_mutex_lock(&g_mutex);
		*p ^= y;
		pthread_mutex_unlock(&g_mutex);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int c, n_threads = 1, i, tmp;
	uint64_t z;
	worker_t *w, w0;
	pthread_t *tid;
	pthread_attr_t attr;

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
	fprintf(stderr, "Lock type: 0 for single-thread; 1 for gcc builtin; 2 for spin lock; 3 for pthread spin; 4 for mutex;\n");
	fprintf(stderr, "           5 for semaphore; 6 for buffer+spin; 7 for buffer+mutex\n");
	
	/* 64 align */
	w0.bits = (uint64_t*)calloc((w0.n + 63) / 64, 8);

	pthread_mutex_init(&g_mutex, 0);
	pthread_attr_init(&attr);
	tid = alloca(sizeof(pthread_t) * n_threads);
	w = alloca(sizeof(worker_t) * n_threads);
	for (i = 0; i < n_threads; ++i) {
		w[i] = w0;
		w[i].start = i; w[i].step = n_threads;
	}
	long long start = get_nanotime();
	for (i = 0; i < n_threads; ++i) pthread_create(&tid[i], &attr, worker, &w[i]);

	for (i = 0; i < n_threads; ++i) pthread_join(tid[i], 0);
	long long utime = get_nanotime() - start;
	for (i = 0, z = 0, tmp = (w0.n + 63)/64; i < tmp; ++i)
		z ^= w0.bits[i];
	fprintf(stderr, "Hash: %llx (should be 0 if -m is even or aaaaaaaaaaaaaaa if -m is odd)\n", (unsigned long long)z);
	fprintf(stderr, "time = %llu\n", utime);
	free(w0.bits);
	return 0;
}
/*
OSX:    2.53 GHz Intel Core 2 Duo; Mac OS X 10.6.8;           gcc 4.2.1
Linux:  2.83 GHz Intel Xeon E5440; Debian 5.0; kernel 2.6.27; gcc 4.3.2; glibc 2.7
Linux2: 2.3  GHz AMD Opteron 8376; ?;          kernel 2.6.18; gcc 4.1.2; glibc 2.5
=========================================================================================================
 Type              OSX-1         OSX-2        Linux-1        Linux-2          Linux-4         Linux2-4
---------------------------------------------------------------------------------------------------------
 Single         1.4/1.4+0.0     (Wrong)     1.2/1.1+0.0      (Wrong)          (Wrong)          (Wrong)
 Atomic         3.8/3.8+0.0   2.2/4.2+0.0   3.6/3.6+0.0    3.7/7.3 +0.0     2.5/9.9 +0.0    6.6/25.5+0.0
 Pseudo spin    5.8/5.7+0.0   5.1/9.5+0.0   5.1/5.1+0.0   20.7/41.1+0.1    32.7/130 +0.3  ~49.6/180 +0.2
 Pthread spin      (N/A)         (N/A)      3.8/3.8+0.0   21.1/42.0+0.2    53.8/206 +0.5  ~40.5/203 +0.0
 Pthread mutex  7.5/7.5+0.0  ~182/66 +205   6.0/5.9+0.0   31.0/22.2+22.9  ~97.2/55.9+228  ~95.5/83.0+237
 Semaphore      ~85/55 +30    ~51/55 +41    5.5/5.4+0.0   46.0/30.4+38.4   ~133/76.4+342  ~98.8/57.2+248
 Buffer+spin    1.3/1.3+0.0   0.7/1.3+0.0   1.2/1.2+0.0    0.7/1.4 +0.0     0.5/2.0 +0.0
 Buffer+mutex   1.3/1.3+0.0   0.7/1.4+0.0   1.2/1.2+0.0    0.7/1.4 +0.0     0.5/1.5 +0.0
=========================================================================================================
* numbers with "~" are measured with a smaller -m and then scaled to -m100.
* CPU time is taken from a couple of runs. Variance seems quite large, though.
 */