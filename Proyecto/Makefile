INCLUDES = includes/
SRCLIB = srclib/
CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : miner monitor

clean :
	rm -f *.o miner monitor *.txt
	rm /dev/shm/facepulls_shm

miner : $(SRCLIB)miner.c $(SRCLIB)pow.c
	$(CC) $(CFLAGS) $^ -o $@

monitor : $(SRCLIB)monitor.c
	$(CC) $(CFLAGS) $^ -o $@

runm:
	./monitor