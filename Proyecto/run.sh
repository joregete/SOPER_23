#!/bin/bash

make clean
make

gnome-terminal -- bash -c "cd "$(pwd)"; valgrind --leak-check=full ./miner 1 1 ; exec bash"
gnome-terminal -- bash -c "cd "$(pwd)"; valgrind --leak-check=full ./miner 1 1 ; exec bash"
gnome-terminal -- bash -c "cd "$(pwd)"; valgrind --leak-check=full ./miner 1 1 ; exec bash"