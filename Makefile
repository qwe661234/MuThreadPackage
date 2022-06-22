CFLAGS = -Wall -fno-stack-protector -I. -pthread -lpthread

ifeq ("$(PTHREAD)","1")
	CFLAGS += -DPTHREAD
	OBJ := Tests/$(FILE).c
else
	OBJ := Tests/$(FILE).c \
		mutex.c \
		thread.c \
		util.c \
		clone.S
endif

all:
	gcc $(CFLAGS) -o test $(OBJ)

clean:
	rm -f test