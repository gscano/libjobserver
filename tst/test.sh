#! /bin/bash

export "MAKEFLAGS=$(echo $MAKEFLAGS | sed 's/-j2 //g')"
echo "MAKEFLAGS:$MAKEFLAGS"

make -f test.mk "$@"
