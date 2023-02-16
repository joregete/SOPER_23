CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : mrush 
clean :
	rm -f *.o mrush

mrush : miner.c monitor.c pow.c mrush.c
	$(CC) $(CFLAGS) $^ -o $@
