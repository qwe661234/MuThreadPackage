CFLAGS = -Wall -fno-stack-protector -I. -pthread -lpthread

ifeq ("$(PTHREAD)","1")
	CFLAGS += -DPTHREAD
	OBJ := Tests/$(FILE).c
else
	OBJ := Tests/$(FILE).c \
		mutex.c \
		condvar.c \
		thread.c \
		util.c \
		clone.S
endif

all:
	gcc $(CFLAGS) -o test-$(NUM) $(OBJ)

check:
	make FILE=test-01-basic NUM=1
	./test-1

test:
	Tests/driver.py

cond:
	make FILE=test-10-condvar-signal NUM=10
clean: 
	rm -f test-*