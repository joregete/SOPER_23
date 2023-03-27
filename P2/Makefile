CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : voting 
clean :
	rm -f *.o voting
	rm -f /dev/shm/sem.calf_raise_sem

voting : voting.c
	$(CC) $(CFLAGS) $^ -o $@

runv: voting
	valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes --track-origins=yes -s ./voting 2 2

run: voting
	./voting 2 2