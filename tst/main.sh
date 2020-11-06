#!/bin/bash

case $1 in
    "one")   export JOBSERVER_TEST+="one";   ./main 0 ./main.sh 1 2 1 2 ;;
    "two")   export JOBSERVER_TEST+="two";   ./main 1 ./main.sh 1 2 1 2 1 2 ;;
    "multi") export MAKEFLAGS=$MAKEFLAGS; export JOBSERVER_TEST+="multi"; ./main ! ./main.sh 1 2 1 2 1 2 1 2;;

    [0-9])  echo "$JOBSERVER_TEST"; sleep $1 ;;

    *) echo "Error" ;;
esac
