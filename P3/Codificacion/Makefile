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
	valgrind --leak-check=full -s ./miner 15 0

runvMonitor1: monitor
	valgrind --leak-check=full ./monitor 500

runvMonitor2: monitor
	valgrind --leak-check=full ./monitor 100
