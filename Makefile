CFLAGS = -Wall -fno-stack-protector -I. -pthread -lpthread
CC := gcc

ifeq ("$(PTHREAD)","1")
	CFLAGS += -DPTHREAD
else
	OBJ := mutex.c \
		condvar.c \
		thread.c \
		util.c \
		clone.S
endif

all:
	$(CC) $(CFLAGS) -o test-01-basic Tests/test-01-basic.c $(OBJ)
	$(CC) $(CFLAGS) -o test-02-multiple-type Tests/test-02-multiple-type.c $(OBJ)
	$(CC) $(CFLAGS) -o test-03-add-count Tests/test-03-add-count.c $(OBJ)
	$(CC) $(CFLAGS) -o test-04-priority-inversion Tests/test-04-priority-inversion.c $(OBJ)
	$(CC) $(CFLAGS) -o test-05-priority-inversion-PI Tests/test-05-priority-inversion-PI.c $(OBJ)
	$(CC) $(CFLAGS) -o test-06-priority-inversion-PP Tests/test-06-priority-inversion-PP.c $(OBJ)
	$(CC) $(CFLAGS) -o test-07-benchmark Tests/test-07-benchmark.c $(OBJ)
	$(CC) $(CFLAGS) -o test-08-benchmark-PI Tests/test-08-benchmark-PI.c $(OBJ)
	$(CC) $(CFLAGS) -o test-09-benchmark-PP Tests/test-09-benchmark-PP.c $(OBJ)
	$(CC) $(CFLAGS) -o test-10-condvar-signal Tests/test-10-condvar-signal.c $(OBJ)
	$(CC) $(CFLAGS) -o test-11-condvar-signal_pi Tests/test-11-condvar-signal_pi.c $(OBJ)
	$(CC) $(CFLAGS) -o test-12-prio-wake Tests/test-12-prio-wake.c $(OBJ)

check:
	$(CC) $(CFLAGS) -o test-01-basic Tests/test-01-basic.c $(OBJ)
	./test-01-basic

test:
	Tests/driver.py

clean: 
	rm -f test-*