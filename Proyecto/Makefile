INCLUDES = includes/
SRCLIB = srclib/
CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : miner

clean :
	rm -f *.o miner monitor

miner : $(SRCLIB)miner.c $(SRCLIB)pow.c
	$(CC) $(CFLAGS) $^ -o $@

