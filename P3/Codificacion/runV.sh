#!/bin/bash

make

# Open a terminal and run valgrind --leak-check=full ./miner 15 0
gnome-terminal -- bash -c "cd "$(pwd)"; valgrind --leak-check=full ./miner 2 0; exec bash"

# Open a terminal and run valgrind --leak-check=full ./monitor 500
gnome-terminal -- bash -c "cd "$(pwd)"; valgrind --leak-check=full ./monitor 500; exec bash"

# Open a terminal and run valgrind --leak-check=full ./monitor 100
gnome-terminal -- bash -c "cd "$(pwd)"; valgrind --leak-check=full ./monitor 100; exec bash"

make clean
