INCLUDES = includes/
SRCLIB = srclib/
CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : miner

clean :
	rm -f *.o miner monitor *.txt

miner : $(SRCLIB)miner.c $(SRCLIB)pow.c
	$(CC) $(CFLAGS) $^ -o $@

