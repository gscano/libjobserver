#!/bin/bash

MAKE=/usr/bin/make

case $1 in
    "one1") export JOBSERVER_TEST="single1";  ./main 0 ./main.sh 1 2 1 2 ;;
    "two1") export JOBSERVER_TEST="two1";     ./main 1 ./main.sh 1 2 1 2 1 2 ;;
    "multi1") export JOBSERVER_TEST="multi1"; ./main ! ./main.sh 1 2 1 2 1 2 1 2;;

    "%") export JOBSERVER_TEST="%"; ./main 2 $MAKE '-f main.mk jtm-1' ;;
    "@") export JOBSERVER_TEST="@"; ./main ! $MAKE '-f main.mk jtm-1' '-f main.mk jtm-2' ;;

    [0-9])  echo "$JOBSERVER_TEST"
	    sleep $1 ;;

    *) echo "Error" ;;
esac
