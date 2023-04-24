INCLUDES = includes/
SRCLIB = srclib/
LAUNCH = launch/
CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : miner monitor

clean :
	rm -f *.o miner monitor *.txt
	rm /dev/shm/facepulls_shm

miner : $(LAUNCH)miner_launch.c $(SRCLIB)pow.c $(SRCLIB)miner.c
	$(CC) $(CFLAGS) $^ -o $@

monitor : $(LAUNCH)monitor_launch.c
	$(CC) $(CFLAGS) $^ -o $@

runmon:
	./monitor

runmin:
	./miner 5 3
