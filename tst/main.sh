#!/bin/bash

#echo "MAKEFLAGS:"$MAKEFLAGS

case $1 in
    "%") ./main ! ./main.sh £ 2 ;;# + AAA AAB AAC AAD ABA ABB ABC ABD AC B
    "@") ./main ! ./main.sh 1 2 1 2 ;;# + A B C D
    "£") ./main ! ./main.sh @ @ 2 ;;# + AA AB AC AD BA BB BC BD C
    *)  echo -n " $JOBSERVER_TEST " >> main.txt
	sleep $1 ;;
esac
