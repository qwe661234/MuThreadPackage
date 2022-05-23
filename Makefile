CFLAGS = -Wall -fno-stack-protector -I.
all:
	gcc $(CFLAGS) -o test \
		test.c \
		mutex.c \
		thread.c \
		util.c \
		clone.S

clean:
	rm -f test