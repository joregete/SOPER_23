CC = gcc -pedantic -pthread
CFLAGS = -Wall -g

all : miner monitor  
clean :
	rm -f *.o miner monitor

miner : miner.c pow.c
	$(CC) $(CFLAGS) $^ -o $@

monitor : monitor.c pow.c
	$(CC) $(CFLAGS) $^ -o $@

runvMiner: miner
	# valgrind --leak-check=full --show-leak-kinds=all -s ./miner <ROUNDS> <LAG>

runvMonitor: monitor
	# valgrind --leak-check=full --show-leak-kinds=all -s ./monitor <LAG>
