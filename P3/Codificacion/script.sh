#!/bin/bash

# Open a terminal and run ./miner 15 0
gnome-terminal -- bash -c "cd "$(pwd)"; ./miner 15 0; exec bash"

# Open a terminal and run ./monitor 500
gnome-terminal -- bash -c "cd "$(pwd)"; ./monitor 500; exec bash"

# Open a terminal and run ./monitor 100
gnome-terminal -- bash -c "cd "$(pwd)"; ./monitor 100; exec bash"