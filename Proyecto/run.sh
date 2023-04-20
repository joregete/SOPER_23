#!/bin/bash

make clean
make

gnome-terminal -- bash -c "cd "$(pwd)"; ./miner 5 3 ; exec bash"
gnome-terminal -- bash -c "cd "$(pwd)"; ./miner 5 3 ; exec bash"
gnome-terminal -- bash -c "cd "$(pwd)"; ./miner 5 3 ; exec bash"
