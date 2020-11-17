#!/bin/bash

if [ -z "$2"]
then
    MAKE=/usr/bin/make
else
    MAKE="$2"
fi

$MAKE -f main.mk $1 MAKE=$MAKE
