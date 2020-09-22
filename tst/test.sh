#!/bin/bash

SLEEP=/usr/bin/sleep

./main 0 $SLEEP 1
./main 0 $SLEEP 1 2 3 4
./main 0 $SLEEP 4 3 2 1

./main 1 $SLEEP 1
./main 1 $SLEEP 1 2 3 4
./main 1 $SLEEP 4 3 2 1

./main 4 $SLEEP 1 1 1 1 1 1 1 1 1 1
./main 4 $SLEEP 1 2 1 2 1 2 1 2 1 2

./main 3 ./main.sh % @ £ @ % 3
