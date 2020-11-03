#!/bin/bash

MAKE=/usr/bin/make

case $1 in
    "+") ./main 1 ./main.sh 1 1 1 1;;# + A B C D (4)
    "-") ./main 2 ./main.sh 2 1 2 ;;# + A B C (5)

    "@") ./main ! ./main.sh 1 2 1 2 ;;# + A B C D (6)
    "%") ./main ! ./main.sh £ 2 ;;# + AAA AAB AAC AAD ABA ABB ABC ABD AC B (16)
    "£") ./main ! ./main.sh @ @ 2 ;;# + AA AB AC AD BA BB BC BD C (14)

    "&") ./main ! $MAKE '-f main.mk called-1' '-f main.mk called-2' ;;
    "=") $MAKE called-2 ;;
    "/") $MAKE call-1 ;;

    *)  echo -n " $JOBSERVER_TEST " >> main.txt
	sleep $1 ;;
esac
