#!/bin/sh
make
./simulator
cp simple_road.dat ../pytest
python3 ../pytest/simple_road.py
