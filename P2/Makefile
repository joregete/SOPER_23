CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : voting 
.PHONY : clean
clean :
	rm -f *.o voting pids voting_file

voting : voting.c
	$(CC) $(CFLAGS) $^ -o $@

runv: voting
	valgrind --leak-check=full --show-leak-kinds=all -s ./voting 3 3

run: voting
	./voting 5 5