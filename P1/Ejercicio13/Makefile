CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : mrush 
clean :
	rm -f *.o mrush

mrush : miner.c monitor.c pow.c mrush.c
	$(CC) $(CFLAGS) $^ -o $@

runv: mrush
	valgrind --leak-check=full --show-leak-kinds=all -s ./mrush 0 5 3

run: mrush
	./mrush 0 5 3