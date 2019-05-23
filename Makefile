


CC=gcc
CFLAGS=-g -Wall -c
LDFLAGS=-lpthread


all: main test

main: fibres.o testfibres.o 
	$(CC) $^ -o $@ $(LDFLAGS) 

main.o: main.c
	$(CC) $(CFLAGS) $^ -o $@
fibres.o: fibres.h fibres.c
	$(CC) $(CFLAGS) fibres.c -o $@ 
testfibres.o: testfibres.c
	$(CC) $(CFLAGS) $^ -o $@
test: test.c
	$(CC) $(CFLAGS) $^ -o test.o
	$(CC) test.o -o $@
clean:
	rm -rf *.o main test
