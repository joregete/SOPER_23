CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : mrush 
clean :
	rm -f *.o mrush

mrush : miner.c monitor.c pow.c mrush.c
	$(CC) $(CFLAGS) $^ -o $@

val: mrush
	valgrind --leak-check=full --show-leak-kinds=all -s ./mrush 0 5 3