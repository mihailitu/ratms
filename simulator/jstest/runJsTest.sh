#!/bin/bash
make -C ../__bin/
rm roads.dat output.dat
../__bin/simulator
node server.js

