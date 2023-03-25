CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : voting 
clean :
	rm -f *.o voting

voting : voting.c
	$(CC) $(CFLAGS) $^ -o $@

runv: voting
	valgrind --leak-check=full --show-leak-kinds=all -s ./voting 3 1