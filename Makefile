TARGETS=homework5 thread_example

CFLAGS=-Wall -g -O0

all: $(TARGETS)

homework5: homework5.c
	gcc $(CFLAGS) -o homework5 homework5.c -lpthread

thread_example: thread_example.c
	gcc $(CFLAGS) -o thread_example thread_example.c -lpthread

clean:
	rm -f $(TARGETS)

