


CC=gcc
CFLAGS=-g -Wall -c
LDFLAGS=-lpthread


all: main

main: fibers.o test.o 
	$(CC) $^ -o $@ $(LDFLAGS) 
fibers.o: fibers.h fibers.c
	$(CC) $(CFLAGS) fibers.c -o $@ 
test.o: test.c 
	$(CC) -c $(CFLAGS) $^ -o $@
clean:
	rm -rf *.o main test
