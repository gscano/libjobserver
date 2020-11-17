#!/bin/bash

mkdir -p make
cd make

wget "https://ftp.gnu.org/gnu/make/make-$1.tar.gz"
tar xzfv "make-$1.tar.gz"

cd "make-$1"

less glob/glob.c | sed 's/_GNU_GLOB_INTERFACE_VERSION >= GLOB_INTERFACE_VERSION/g' > glob/glob.tmp
mv glob/glob.tmp glob/glob.c

./configure --prefix=$(pwd)
make -j 2
