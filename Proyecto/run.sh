#!/bin/bash

make clean
make

gnome-terminal -- bash -c "cd "$(pwd)"; make runmin ; exec bash"
gnome-terminal -- bash -c "cd "$(pwd)"; make runmin ; exec bash"
gnome-terminal -- bash -c "cd "$(pwd)"; make runmin ; exec bash"
gnome-terminal -- bash -c "cd "$(pwd)"; make runmon ; exec bash"
